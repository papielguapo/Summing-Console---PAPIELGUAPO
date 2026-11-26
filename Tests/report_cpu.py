from pathlib import Path
import csv,sys,json,hashlib
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
out=Path(sys.argv[1]); rows=list(csv.DictReader((out/'cpu.csv').open()))
plt.rcParams.update({'figure.facecolor':'#101820','axes.facecolor':'#121e28','axes.edgecolor':'#35495b','text.color':'#d4e7ee','axes.labelcolor':'#a8c1cf','xtick.color':'#a8c1cf','ytick.color':'#a8c1cf','grid.color':'#2b3e4c','font.family':'DejaVu Sans'})
if rows and 'oversampling' in rows[0]:
    fig,axs=plt.subplots(1,2,figsize=(12,5),layout='constrained')
    for ax,rate in zip(axs,[48000,96000]):
        data=[r for r in rows if int(r['sample_rate'])==rate]
        med=np.array([float(r['median_deadline_percent']) for r in data]); p95=np.array([float(r['p95_deadline_percent']) for r in data])
        ax.bar(range(3),med,color=['#42ed95','#41d2f6','#ffb32d']); ax.errorbar(range(3),med,yerr=[np.zeros(3),p95-med],fmt='none',ecolor='#d4e7ee',capsize=6)
        for i,y in enumerate(med): ax.text(i,y+1,f'{y:.1f}%',ha='center')
        ax.set_xticks(range(3)); ax.set_xticklabels(['0x / native','4x','8x']); ax.set_ylim(0,max(65,max(p95)+6)); ax.grid(True,axis='y',alpha=.4); ax.set_title(f'{rate//1000} kHz / 256 samples'); ax.set_ylabel('Single-core audio deadline %')
    fig.suptitle('CPU 2.1 / median and P95 / stereo / no GUI or DAW')
    fig.savefig(out/'tone_2.1_cpu.png',dpi=150); plt.close(fig)
    sys.exit(0)

fig,axs=plt.subplots(1,2,figsize=(14,6),layout='constrained')
for ax,rate in zip(axs,[48000,96000]):
    xx=np.arange(3)
    for setting,c,shift in [('Jaycen_default','#ffb32d',-.18),('HEAT_D10_T10','#41d2f6',.18)]:
        data=[r for r in rows if int(r['sample_rate'])==rate and r['setting']==setting]
        med=np.array([float(r['median_deadline_percent']) for r in data]);p95=np.array([float(r['p95_deadline_percent']) for r in data])
        ax.bar(xx+shift,med,width=.34,color=c,label=setting.replace('_',' '));ax.errorbar(xx+shift,med,yerr=np.array([np.zeros(3),p95-med]),fmt='none',ecolor=c,capsize=5)
        for x,y in zip(xx+shift,med):ax.text(x,y+1,f'{y:.1f}%',ha='center',color=c,fontsize=9)
    ax.set_title(f'{rate//1000} kHz · estéreo · 8x IIR',color='#54d6f6',weight='bold');ax.set_xticks(xx);ax.set_xticklabels(['64','256','512']);ax.set_xlabel('Tamaño del bloque (muestras)');ax.set_ylabel('% del tiempo disponible de un núcleo');ax.set_ylim(0,100);ax.grid(True,axis='y',alpha=.4);ax.legend(fontsize=9)
fig.suptitle('CPU MEDIDA · Intel Core i7-9750H @ 2.60 GHz\nMediana y P95 (bigotes) · ruido y AutoGain ON · sin GUI/DAW',color='#ffb32d',weight='bold',fontsize=15)
fig.savefig(out/'03_consumo_cpu.png',dpi=150);plt.close(fig)
