from pathlib import Path
import sys, json, csv
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

root=Path(sys.argv[1]); out=Path(sys.argv[2]); out.mkdir(parents=True,exist_ok=True)
plt.rcParams.update({'figure.facecolor':'#101820','axes.facecolor':'#121e28','axes.edgecolor':'#35495b','text.color':'#d4e7ee','axes.labelcolor':'#a8c1cf','xtick.color':'#a8c1cf','ytick.color':'#a8c1cf','grid.color':'#2b3e4c','font.family':'DejaVu Sans'})
names=['Jaycen 8x','SSL Spike 8x','Neve Nevo 8x','EMI Mike 8x','HEAT Hybrid 8x']
colours={-10:'#41d2f6',-5:'#a0ddeb',0:'#d6dce4',5:'#df9b5c',10:'#ffad2b'}
labels={-10:'-10 Open & Tight',-5:'-5',0:'0 Referencia',5:'+5',10:'+10 Weight & Warm'}
def load(name): return np.fromfile(root/(name+'.f32'),dtype='<f4').astype(float).reshape(-1,2)
def db(x): return 20*np.log10(np.maximum(abs(x),1e-16))
def response(name):
    x=load(name); return np.fft.rfftfreq(len(x),1/48000),np.fft.rfft(x,axis=0)/1.e-4
m={'version':'2.2.0','sample_rate':48000,'variants':[],'checks':[]}
def check(name,passed): m['checks'].append({'name':name,'pass':bool(passed)})
fig=plt.figure(figsize=(16,12),layout='constrained'); gs=fig.add_gridspec(3,2,height_ratios=[1.3,1,1])
axes=[fig.add_subplot(gs[0,:])]+[fig.add_subplot(gs[r,c]) for r in [1,2] for c in [0,1]]
for i,(name,ax) in enumerate(zip(names,axes)):
    row={'name':name,'tones':{}}
    for tone in [-10,-5,0,5,10]:
        f,h=response(f'tone21_fr_{i}_{tone}'); curve=db(h[:,0])
        ax.plot(f,curve,c=colours[tone],lw=2 if tone in [-10,0,10] else 1,ls='-' if tone in [-10,0,10] else '--',label=labels[tone])
        row['tones'][str(tone)]={str(hz):float(curve[np.argmin(abs(f-hz))]) for hz in [5,10,20,27,53,92,100,1000,8000,16000,20000]}
        if tone in [-10,0,10]:
            x=load(f'tone21_ag_{i}_{tone}'); error=float(db(np.sqrt(np.mean(x[:,0]**2))))+18
            row['tones'][str(tone)]['ag_rms_error_db']=error
            check(f'{name}: AutoGain Tone {tone:+} within 0.1 dB',abs(error)<.1)
    delta=float(np.max(abs(load(f'tone21_fr_{i}_0')-load(f'tone20_center_{i}')))); row['center_max_sample_difference']=delta
    check(name+': center identical to v2.0',delta<1.e-10)
    left,center,right=[row['tones'][str(t)] for t in [-10,0,10]]
    check(name+': left removes 92 Hz dip',left['92']>center['92']+.3 and abs(left['92'])<.4)
    check(name+': left opens highs',left['16000']>center['16000']+.8)
    check(name+': right softens highs',right['16000']<center['16000']-.6)
    check(name+': left controls 27 Hz',left['27']<center['27']-.35)
    check(name+': right adds controlled weight',abs(right['27']-center['27'])<.15 and right['53']>center['53']+.05)
    check(name+': right does not deepen dip',right['92']>=center['92']-.15)
    check(name+': left reduces punch contrast',left['53']<center['53']-.05)
    check(name+': bass gain below 3.5 dB',max(right[k] for k in ['27','53','92'])<3.5)
    ax.set_xscale('log'); ax.set_xlim(20,20000); ax.set_ylim(-5,3.5); ax.grid(True,alpha=.4); ax.axhline(0,c='#627685',lw=.6)
    ax.set_xticks([20,27,53,92,200,1000,5000,16000]); ax.xaxis.set_major_formatter(FuncFormatter(lambda v,p:f'{v/1000:g}k' if v>=1000 else f'{v:g}'))
    ax.set_title(name,color='#ffb32d',weight='bold'); ax.set_ylabel('Ganancia (dB)'); ax.set_xlabel('Frecuencia (Hz)')
    if i==0: ax.legend(ncol=5,fontsize=10,loc='upper right')
    m['variants'].append(row)
fig.suptitle('TONE 2.2 · IZQUIERDA ABIERTA / DERECHA CALIDA\nHP fijo 21 Hz · 8x IIR · centro conservado',fontsize=18,color='#ffb32d',weight='bold')
fig.savefig(out/'tone_2.2_respuesta.png',dpi=150); plt.close(fig)
m['aliasing']={}
for factor in [1,4,8]:
    for tone in [-10,10]:
        x=load(f'tone21_alias_{factor}_{tone}')[:,0]; spec=abs(np.fft.rfft(x)); bins=[1000,3000,4000,7000,8000,10000,12000,14000,15000,18000,19000,21000,23000]
        value=float(db(np.linalg.norm(spec[bins])/spec[11000])); m['aliasing'][f'{factor}x_tone_{tone}']=value
        if factor>=4: check(f'{factor}x alias Tone {tone:+} below -80 dBc',value<-80)
if (out/'cpu.csv').exists():
    rows=list(csv.DictReader((out/'cpu.csv').open())); m['cpu']=rows
    fig,axs=plt.subplots(1,2,figsize=(12,5),layout='constrained')
    for ax,rate in zip(axs,[48000,96000]):
        data=[r for r in rows if int(r['sample_rate'])==rate]
        yy=[float(r['median_deadline_percent']) for r in data]; pp=[float(r['p95_deadline_percent']) for r in data]
        ax.bar(range(3),yy,color=['#42ed95','#41d2f6','#ffb32d']); ax.errorbar(range(3),yy,yerr=[np.zeros(3),np.array(pp)-yy],fmt='none',ecolor='#d4e7ee',capsize=6)
        for j,y in enumerate(yy): ax.text(j,y+1,f'{y:.1f}%',ha='center',color='#d4e7ee')
        ax.set_xticks(range(3)); ax.set_xticklabels(['0x / nativo','4x','8x']); ax.set_ylim(0,max(65,max(pp)+6)); ax.grid(True,axis='y',alpha=.4); ax.set_ylabel('% del plazo de un nucleo'); ax.set_title(f'{rate//1000} kHz · 256 muestras',color='#ffb32d')
        check(f'{rate} Hz: native < 4x < 8x CPU',yy[0]<yy[1]<yy[2])
    fig.suptitle('CPU 2.2 · Intel Core i7-9750H · estereo\nMediana y P95 · ruido y AutoGain ON · sin GUI/DAW',fontsize=15,color='#ffb32d')
    fig.savefig(out/'tone_2.2_cpu.png',dpi=150); plt.close(fig)
(out/'tone_2.2_mediciones.json').write_text(json.dumps(m,indent=2),encoding='utf-8')
with (out/'tone_2.2_respuesta.csv').open('w',newline='',encoding='utf-8') as file:
    w=csv.writer(file); w.writerow(['variant','tone','frequency_hz','gain_db'])
    for row in m['variants']:
        for tone,values in row['tones'].items():
            for freq,gain in values.items():
                if freq.isdigit(): w.writerow([row['name'],tone,freq,gain])
for c in m['checks']: print(('PASS ' if c['pass'] else 'FAIL ')+c['name'])
print(json.dumps({'aliasing':m['aliasing'],'centers':[v['center_max_sample_difference'] for v in m['variants']]},indent=2))
if not all(c['pass'] for c in m['checks']): sys.exit(1)
