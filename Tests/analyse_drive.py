from pathlib import Path
import sys,json,csv
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

root=Path(sys.argv[1]);out=Path(sys.argv[2]);out.mkdir(parents=True,exist_ok=True)
plt.rcParams.update({'figure.facecolor':'#101820','axes.facecolor':'#121e28','axes.edgecolor':'#35495b','text.color':'#d4e7ee','axes.labelcolor':'#a8c1cf','xtick.color':'#a8c1cf','ytick.color':'#a8c1cf','grid.color':'#2b3e4c','font.family':'DejaVu Sans'})
names=['Jaycen','SSL Spike','Neve Nevo','EMI Mike','HEAT Hybrid']
colours=['#ffb32d','#41d2f6','#be80ff','#42ed95','#ff527c']
drives=[-10,-8,-5,-2,0,1,5,10]
def load(name):return np.fromfile(root/(name+'.f32'),dtype='<f4').astype(float).reshape(-1,2)
def db(x):return 20*np.log10(np.maximum(abs(x),1.e-18))
def harmonics(x):
    spec=abs(np.fft.rfft(x[:,0]));h=spec[np.arange(1,17)*1000]
    return db(h/h[0]),float(db(np.linalg.norm(h[1:])/h[0]))
m={'version':'2.1.1','sample_rate':48000,'oversampling':8,'tone':0,'input_rms_dbfs':-18,'noise':False,'autogain':False,'variants':[],'checks':[]}
def check(name,value):m['checks'].append({'name':name,'pass':bool(value)})
fig=plt.figure(figsize=(15,10),layout='constrained');gs=fig.add_gridspec(2,2)
ax=fig.add_subplot(gs[0,:])
for i,name in enumerate(names):
    row={'name':name,'drive':{}};values=[]
    for drive in drives:
        x=load(f'drive211_{i}_{drive}');h,thd=harmonics(x);values.append(thd)
        row['drive'][str(drive)]={'thd_dbc':thd,'h2_h16_dbc':h[1:].tolist()}
    ax.plot(drives,values,'-o',c=colours[i],label=name,lw=2)
    if i<4:
        check(name+': -10 below -125 dBc numerical floor',values[0]<-125)
        check(name+': strictly increasing THD with Drive',bool(np.all(np.diff(values)>.1)))
        for drive in [0,1,5,10]:
            difference=float(np.max(abs(load(f'drive211_{i}_{drive}')-load(f'drive_ref_{i}_{drive}'))))
            row['drive'][str(drive)]['baseline_max_difference']=difference
            check(f'{name}: positive/reference {drive:+} preserved',difference<1.e-7)
        clean=load(f'drive_fr_{i}_-10');coloured=load(f'drive_fr_{i}_1')
        f=np.fft.rfftfreq(len(clean),1/48000);a=np.fft.rfft(clean,axis=0);b=np.fft.rfft(coloured,axis=0);mask=(f>=20)&(f<=20000)
        error=float(np.max(abs(db(a[mask])-db(b[mask]))));row['filter_response_difference_db']=error
        check(name+': clean endpoint preserves impedance response within 0.005 dB',error<.005)
    else:
        for drive in [-10,-5,0,1,5,10]:
            difference=float(np.max(abs(load(f'drive211_{i}_{drive}')-load(f'drive_ref_{i}_{drive}'))))
            row['drive'][str(drive)]['baseline_max_difference']=difference
            check(f'HEAT: bipolar Drive {drive:+} preserved',difference<1.e-7)
        check('HEAT: negative -10 remains nonlinear by design',values[0]>-90)
    m['variants'].append(row)
ax.set_xlabel('Drive');ax.set_ylabel('THD H2-H16 (dBc)');ax.set_ylim(-175,-35);ax.set_xticks(drives);ax.grid(True,alpha=.4);ax.legend(ncol=3);ax.set_title('Consolas: -10 lineal / saturacion progresiva · HEAT conserva ley bipolar',color='#54d6f6')
ax=fig.add_subplot(gs[1,0]);xx=np.arange(2,17)
for drive,c,shift in [(-10,'#42ed95',-.27),(-5,'#41d2f6',-.09),(0,'#b8c9d6',.09),(10,'#ffb32d',.27)]:
    h=np.array(m['variants'][0]['drive'][str(drive)]['h2_h16_dbc']);ax.bar(xx+shift,np.maximum(h,-175)+175,width=.18,bottom=-175,color=c,label=f'Drive {drive:+}')
ax.set_ylim(-175,-35);ax.set_xticks(xx);ax.set_xticklabels([f'H{k}' for k in xx],fontsize=8);ax.set_ylabel('Armonicos Jaycen (dBc)');ax.grid(True,axis='y',alpha=.4);ax.legend(fontsize=8)
ax=fig.add_subplot(gs[1,1])
for drive,c,style in [(-10,'#42ed95','-'),(1,'#ffb32d','--')]:
    x=load(f'drive_fr_0_{drive}');f=np.fft.rfftfreq(len(x),1/48000);h=np.fft.rfft(x[:,0])/1.e-4;ax.plot(f,db(h),c=c,ls=style,lw=2,label=f'Drive {drive:+}')
ax.set_xscale('log');ax.set_xlim(20,20000);ax.set_ylim(-4,3);ax.set_xlabel('Frecuencia (Hz)');ax.set_ylabel('Ganancia (dB)');ax.grid(True,alpha=.4);ax.legend();ax.set_title('Los filtros permanecen: curvas superpuestas',color='#54d6f6')
fig.suptitle('DRIVE 2.1.1 · CORRECCION DE LA LEY NEGATIVA\n1 kHz a -18 dBFS RMS · 8x · Tone 0 · ruido y AutoGain OFF',fontsize=17,color='#ffb32d',weight='bold')
fig.savefig(out/'drive_2.1.1_armonicos.png',dpi=150);plt.close(fig)
(out/'drive_2.1.1_mediciones.json').write_text(json.dumps(m,indent=2),encoding='utf-8')
with (out/'drive_2.1.1_mediciones.csv').open('w',newline='',encoding='utf-8') as file:
    w=csv.writer(file);w.writerow(['variant','drive','thd_h2_h16_dbc'])
    for v in m['variants']:
        for d,result in v['drive'].items():w.writerow([v['name'],d,result['thd_dbc']])
for c in m['checks']:print(('PASS ' if c['pass'] else 'FAIL ')+c['name'])
print(json.dumps({v['name']:{d:round(r['thd_dbc'],2) for d,r in v['drive'].items()} for v in m['variants']},indent=2))
if not all(c['pass'] for c in m['checks']):sys.exit(1)
