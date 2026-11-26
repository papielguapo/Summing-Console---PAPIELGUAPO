from pathlib import Path
import sys, json, math, csv
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter
root=Path(sys.argv[1]); out=Path(sys.argv[2]); out.mkdir(parents=True,exist_ok=True)
FS=48000
names=['Jaycen 8x','SSL Spike 8x','Neve Nevo 8x','EMI Mike 8x','HEAT Hybrid 8x']
colours=['#ffb32d','#41d2f6','#be80ff','#42ed95','#ff527c']
plt.rcParams.update({'figure.facecolor':'#101820','axes.facecolor':'#121e28','axes.edgecolor':'#35495b','text.color':'#d4e7ee','axes.labelcolor':'#a8c1cf','xtick.color':'#a8c1cf','ytick.color':'#a8c1cf','grid.color':'#2b3e4c','font.family':'DejaVu Sans','font.size':10,'savefig.facecolor':'#101820'})
def db(x): return 20*np.log10(np.maximum(np.abs(x),1e-16))
def load(name): return np.fromfile(root/(name+'.f32'),dtype='<f4').astype(float).reshape(-1,2)
def response(name):
    x=load(name); return np.fft.rfftfreq(len(x),1/FS),np.fft.rfft(x,axis=0)/1e-4

def harmonic(name):
    x=load(name); spec=np.abs(np.fft.rfft(x,axis=0))*2/len(x)
    h=spec[np.arange(1,17)*1000]
    thd=float(db(np.linalg.norm(h[1:,0])/h[0,0]))
    return db(h[:,0]/h[0,0]),thd,float(db(h[0,0])),float(db(np.sqrt(np.mean(x[:,0]**2))))

def freqaxis(ax):
    ax.set_xscale('log');ax.set_xlim(20,20000)
    ticks=[20,27,53,92,200,500,1000,2000,5000,10000,20000]
    ax.set_xticks(ticks);ax.xaxis.set_major_formatter(FuncFormatter(lambda v,p:f'{v/1000:g}k' if v>=1000 else f'{v:g}'))
    ax.grid(True,alpha=.5);ax.set_xlabel('Frecuencia (Hz)');ax.set_ylabel('Ganancia (dB)')
metrics={'sample_rate_hz':FS,'new_oversampling':8,'legacy_oversampling':4,'nominal_input':'-18 dBFS RMS / -14.99 dBFS peak','hot_inputs':'-6 and -3 dBFS peak','noise':False,'autogain':False,'variants':[]}
fig=plt.figure(figsize=(18,13),layout='constrained');gs=fig.add_gridspec(3,3,height_ratios=[1.45,1,1])
ax=fig.add_subplot(gs[0,:])
for i,name in enumerate(names):
    f,h=response(f'new_fr_{i}');ax.plot(f,db(h[:,0]),c=colours[i],lw=1.8,label=name)
f,h=response('old_fr_0');ax.plot(f,db(h[:,0]),c='#93a6b0',ls='--',lw=1.3,label='Jaycen anterior (4x)')
for tone,label,c in [(-10,'Jaycen Tone -10 / Tight Sub','#c0e0e8'),(10,'Jaycen Tone +10 / Phat & Air','#ffeb4a')]:
    f,h=response(f'new_tone_{tone}');ax.plot(f,db(h[:,0]),c=c,ls=':',label=label,lw=1.5)
freqaxis(ax);ax.set_ylim(-10,11);ax.axhline(0,c='#647b8b',lw=.7);ax.legend(ncol=3,fontsize=9,loc='upper right');ax.set_title('RESPUESTA MEDIDA · impulso a -80 dBFS · 8 etapas · L (R tiene tolerancias independientes)',loc='left',color='#54d6f6',weight='bold')
for i,name in enumerate(names):
    a=fig.add_subplot(gs[1+i//3,i%3]);h,thd,_,_=harmonic(f'new_h_{i}_r18');old,oldthd,_,_=harmonic(f'old_h_{i}_r18')
    xx=np.arange(2,17);a.bar(xx-.18,np.maximum(h[1:],-160)+160,width=.34,bottom=-160,color=colours[i],label=f'Nuevo THD {thd:.1f} dB')
    a.bar(xx+.18,np.maximum(old[1:],-160)+160,width=.34,bottom=-160,color='#5d7385',label=f'Anterior {oldthd:.1f} dB')
    a.set_title(name,color=colours[i],weight='bold');a.set_ylim(-160,-25);a.set_xticks(xx);a.set_xticklabels([f'H{k}' for k in xx],fontsize=8);a.set_ylabel('Armónicos (dBc)');a.grid(True,axis='y',alpha=.5);a.legend(fontsize=8,loc='upper right')
    row={'name':name,'new_thd_db':thd,'old_thd_db':oldthd,'levels':{}}
    for level in ['r18','p6','p3']:
        _,newthd,fund,rms=harmonic(f'new_h_{i}_{level}');_,oldthd,_,_=harmonic(f'old_h_{i}_{level}')
        row['levels'][level]={'new_thd_db':newthd,'old_thd_db':oldthd,'new_fundamental_dbfs_peak':fund,'new_output_rms_dbfs':rms}
    for version in ['old','new']:
        f,h=response(f'{version}_fr_{i}'); row[version+'_response_db']={str(hz):float(db(h[np.argmin(abs(f-hz)),0])) for hz in [5,10,20,27,40,53,60,92,1000,16000,20000]}
    ag=load(f'new_ag_{i}');row['autogain_rms_error_db']=float(db(np.sqrt(np.mean(ag[:,0]**2))))+18.0
    f,h=response(f'new_fr_{i}'); select=(f>=20)&(f<=20000)
    row['max_lr_gain_difference_db']=float(np.max(abs(db(h[select,0])-db(h[select,1]))))
    metrics['variants'].append(row)
a=fig.add_subplot(gs[2,2]);xx=np.arange(2,17)
for drive,c,shift in [(8,'#ffae2c',-.18),(-8,'#41d2f6',.18)]:
    h,thd,_,_=harmonic(f'new_heat_{drive}');a.bar(xx+shift,np.maximum(h[1:],-160)+160,width=.34,bottom=-160,color=c,label=f'Drive {drive:+} | THD {thd:.1f} dB')
a.set_title('HEAT · Drive ±8 · Tone +10',color='#ff527c',weight='bold');a.set_xticks(xx);a.set_xticklabels([f'H{k}' for k in xx],fontsize=8);a.set_ylim(-160,-25);a.grid(True,axis='y',alpha=.5);a.set_ylabel('Armónicos (dBc)');a.legend(fontsize=8)
fig.suptitle('SUMMING CONSOLE · NUEVA ARQUITECTURA DSP / MEDICIONES REALES\n1 kHz a -18 dBFS RMS · 48 kHz · ruido OFF · AutoGain OFF · 8x IIR',fontsize=17,color='#ffb32d',weight='bold')
fig.savefig(out/'01_respuesta_y_armonicos.png',dpi=150);plt.close(fig)
fig,axs=plt.subplots(2,2,figsize=(17,10),layout='constrained')
a=axs[0,0]
for i,name in enumerate(names):
    row=metrics['variants'][i];levels=row['levels'];a.plot([-18,-9.0103,-6.0103],[levels[x]['new_thd_db'] for x in ['r18','p6','p3']],'-o',color=colours[i],label=name)
a.set_xlabel('Nivel de entrada (dBFS RMS)');a.set_ylabel('THD H2-H16 (dBc)');a.set_title('La distorsión crece con el nivel de entrada');a.grid(True,alpha=.5);a.legend(fontsize=8)
a=axs[0,1]
for factor,c in [(0,'#758b9c'),(3,'#ffb32d'),(4,'#42ed95')]:
    x=load(f'alias_{factor}')[:,0];spec=np.abs(np.fft.rfft(x))*2/len(x);freq=np.fft.rfftfreq(len(x),1/FS)
    a.plot(freq,db(spec/spec[11000]),color=c,lw=.8,label=f'{1<<factor}x')
    aliasbins=[15000,4000,7000,18000,19000,8000,3000,14000,23000,12000,1000,10000,21000]
    metrics[f'alias_{1<<factor}x_folded_harmonics_dbc']=float(db(np.linalg.norm(spec[aliasbins])/spec[11000]))
a.set_xlim(0,24000);a.set_ylim(-170,5);a.set_title('Aliasing · 11 kHz / -3 dBFS pico · Drive +10, Tone +10');a.set_xlabel('Frecuencia (Hz)');a.set_ylabel('Espectro (dBc)');a.legend();a.grid(True,alpha=.5)
a=axs[1,0]
for stem,label,c in [('old_fr_0','Anterior Jaycen (4x)','#71899b'),('new_fr_0','Nuevo Jaycen','#ffb32d'),('new_tone_-10','Nuevo Tight Sub','#41d2f6'),('new_tone_10','Nuevo Phat & Air','#ffe54f')]:
    f,h=response(stem);a.plot(f,db(h[:,0]),color=c,label=label)
a.set_xscale('log');a.set_xlim(5,200);a.set_ylim(-25,10);a.set_xticks([5,10,20,27,40,53,60,92,200]);a.xaxis.set_major_formatter(FuncFormatter(lambda v,p:f'{v:g}'));a.set_title('Headroom subgrave: HP 21–22 Hz + DC 5 Hz');a.set_xlabel('Frecuencia (Hz)');a.set_ylabel('Ganancia (dB)');a.grid(True,alpha=.5);a.legend(fontsize=8)
a=axs[1,1]
for i,name in enumerate(names):
    f,h=response(f'new_fr_{i}');a.plot(f,db(h[:,0])-db(h[:,1]),color=colours[i],label=name)
a.set_xscale('log');a.set_xlim(20,20000);a.set_title('Diferencia L−R con entrada dual mono idéntica');a.set_xlabel('Frecuencia (Hz)');a.set_ylabel('Diferencia de ganancia (dB)');a.grid(True,alpha=.5);a.legend(fontsize=8)
a.set_ylim(-0.10,0.10)
fig.suptitle('VALIDACIÓN · NIVEL, ALIASING, SUBGRAVES Y TOLERANCIAS ESTÉREO',fontsize=17,color='#ffb32d',weight='bold')
fig.savefig(out/'02_validacion_dsp.png',dpi=150);plt.close(fig)
(out/'mediciones.json').write_text(json.dumps(metrics,indent=2),encoding='utf-8')
with (out/'mediciones.csv').open('w',newline='',encoding='utf-8') as f:
    w=csv.writer(f);w.writerow(['variant','input','new_thd_dbc','old_thd_dbc','output_rms_dbfs'])
    for row in metrics['variants']:
        for level,vals in row['levels'].items(): w.writerow([row['name'],level,vals['new_thd_db'],vals['old_thd_db'],vals['new_output_rms_dbfs']])
# Numerical acceptance criteria are checked against the captured audio, never chart annotations.
checks=[]
for row in metrics['variants']:
    checks.extend([(row['name']+' nominal THD -70..-60 dB',-70<=row['new_thd_db']<=-60),
                   (row['name']+' THD rises for hot peaks',row['levels']['p3']['new_thd_db']>row['new_thd_db']+6),
                   (row['name']+' steady auto-gain within 0.1 dB',abs(row['autogain_rms_error_db'])<.1),
                   (row['name']+' L/R tolerances under 0.3 dB',row['max_lr_gain_difference_db']<.3)])
checks.append(('8x alias foldback below -85 dBc',metrics['alias_8x_folded_harmonics_dbc']<-85))
(out/'acceptance.json').write_text(json.dumps([{'check':k,'pass':bool(v)} for k,v in checks],indent=2))
for k,v in checks: print(('PASS' if v else 'FAIL')+' '+k)
print(json.dumps(metrics,indent=2))
if not all(v for k,v in checks): sys.exit(1)
