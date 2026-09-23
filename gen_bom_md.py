#!/usr/bin/env python3
# Genera BOM_aliexpress.md (EN) y BOM_aliexpress.es.md (ES) desde los CSV de partes.
# Uso: python3 gen_bom_md.py
# Precios concretados a mano (variación elegida, lote mínimo). Actualizar D si cambian.
import csv, re, math, os, datetime

HERE = os.path.dirname(os.path.abspath(__file__))

# (precio_eur, unidades_por_lote, estado) ; estado: ok | pcb | nolink
D = {
 'parts-list-petalot.csv': {0:(1.75,50,'ok'),1:(1.28,50,'ok'),2:(2.94,30,'ok'),3:(2.04,30,'ok'),4:(0.26,None,'ok'),5:(3.69,10,'ok'),6:(2.69,10,'ok'),7:(3.39,1,'ok'),8:(3.39,20,'ok'),9:(1.94,50,'ok'),10:(3.49,200,'ok'),11:(2.54,50,'ok'),12:(2.54,1,'ok'),13:(3.69,1,'ok')},
 'parts-list-electronics.csv': {0:(None,None,'pcb'),1:(6.09,None,'ok'),2:(2.99,None,'ok'),3:(4.99,10,'ok'),4:(2.13,5,'ok'),5:(1.50,None,'ok'),6:(24.59,None,'ok'),7:(3.09,10,'ok'),8:(3.65,None,'ok'),9:(2.27,50,'ok'),10:(3.79,20,'ok'),11:(0.83,10,'ok'),12:(1.70,100,'ok'),13:(2.18,100,'ok'),14:(0.72,1,'ok'),15:(1.62,1,'ok'),16:(2.36,None,'ok'),17:(1.06,100,'ok'),18:(1.06,100,'ok'),19:(0.94,50,'ok'),20:(0.91,50,'ok'),21:(1.32,1,'ok'),22:(1.23,1,'ok')},
 'parts-list-cutter.csv': {0:(3.69,10,'ok'),1:(2.34,10,'ok'),2:(1.34,10,'ok'),3:(3.39,20,'ok'),4:(3.29,2,'ok'),5:(3.59,3,'ok'),6:(None,None,'nolink'),7:(2.94,30,'ok')},
 'parts-list-tools.csv': {0:(2.03,10,'ok'),1:(5.69,None,'ok'),2:(11.19,None,'ok'),3:(5.59,None,'ok'),4:(9.19,None,'ok'),5:(9.29,None,'ok')},
}
FILES = list(D.keys())
HEAD = {
 'parts-list-petalot.csv':   ('PETALOT machine',       'Máquina PETALOT'),
 'parts-list-electronics.csv':('Electronics for PETALOT','Electrónica para PETALOT'),
 'parts-list-cutter.csv':    ('Bottle Cutter',          'Cortador de botellas'),
 'parts-list-tools.csv':     ('Tools',                  'Herramientas'),
}
STAT = {'ok':('OK','OK'),'pcb':('PCB (JLC, ignored)','PCB (JLC, ignorada)'),'nolink':('NO LINK (M6 rod)','SIN ENLACE (varilla M6)')}
COLS = [['Row','Qty','Component','Link','Variation','Price'],
        ['Fila','Cant.','Componente','Enlace','Variación','Precio']]
TOT_HEAD = ['Totals','Totales']
TOT_COLS = [['List','Total'],['Lista','Total']]
TOTAL_LBL = ['TOTAL (minimum, no shipping)','TOTAL (mínimo, sin envío)']
TOT_NOTE = ['*The PCB is not included in the total (it is fabricated at JLCPCB). The M6 threaded rod for the Bottle Cutter is also not included: no valid AliExpress product was found.*',
            '*La PCB no está incluida en el total (se fabrica en JLCPCB). La varilla roscada M6 del cortador tampoco: no se encontró un producto válido en AliExpress.*']
PCB_PRODUCT = 'https://function3d.xyz/product/pcb-for-petalot'

def _today(lang):
    d = datetime.date.today()
    return d.isoformat() if lang==0 else d.strftime('%d/%m/%Y')

def strip(h):
    h=re.sub(r'<br\s*/?>',' ',h); h=re.sub(r'<[^>]+>','',h); return re.sub(r'\s+',' ',h).strip()
def esc(s): return (s or '').replace('|','\\|')
def eur(v,comma): return (('%.2f'%v).replace('.',',') if comma else ('%.2f'%v))+' €'
def qty_num(q):
    q=str(q)
    if '+' in q: return sum(int(x) for x in re.findall(r'\d+',q))
    if q.strip().endswith(('cm','mm')): return 1
    m=re.match(r'(\d+)',q); return int(m.group(1)) if m else 1

def build(lang, csvname):
    comma = (lang==1)
    # Primera pasada: totales
    totals={}; grand=0.0; cache={}
    for f in FILES:
        rows=[r for r in csv.reader(open(os.path.join(HERE,csvname(f)),newline='',encoding='utf-8')) if r][1:]
        cache[f]=rows
        t=0.0
        for i,row in enumerate(rows):
            price,lot,st = D[f].get(i,(None,None,'ok'))
            if price is None: continue
            qn=qty_num(row[0])
            t += (math.ceil(qn/lot)*price) if (isinstance(lot,int) and lot>0) else (price*qn)
        totals[f]=t; grand+=t
    L=[]
    # Fecha de los precios
    if lang==0:
        L.append(f"Prices updated: **{_today(0)}**. Indicative values, they may vary over time — check AliExpress for the current price.")
    else:
        L.append(f"Precios actualizados: **{_today(1)}**. Valores orientativos, pueden variar con el tiempo — consulta el precio actual en AliExpress.")
    L.append("")
    # Totales (al principio)
    L.append(f"## {TOT_HEAD[lang]}"); L.append("")
    L.append("| "+" | ".join(TOT_COLS[lang])+" |"); L.append("|---|---|")
    for f in FILES: L.append(f"| {HEAD[f][lang]} | {eur(totals[f],comma)} |")
    L.append(f"| **{TOTAL_LBL[lang]}** | **{eur(grand,comma)}** |"); L.append("")
    L.append(TOT_NOTE[lang]); L.append("")
    # Tablas por CSV
    for f in FILES:
        rows=cache[f]
        L.append(f"## {HEAD[f][lang]}"); L.append("")
        L.append("| "+" | ".join(COLS[lang])+" |")
        L.append("|"+"---|"*len(COLS[lang]))
        for i,row in enumerate(rows):
            price,lot,st = D[f].get(i,(None,None,'ok'))
            qn=qty_num(row[0]); var=row[4] if len(row)>4 else ''
            link = row[3]
            if f=='parts-list-electronics.csv' and i==0:
                link = 'https://jlcpcb.com/?from=FUNC · ' + PCB_PRODUCT
            if price is None:
                precio=''
            else:
                total = (math.ceil(qn/lot)*price) if (isinstance(lot,int) and lot>0) else (price*qn)
                precio=eur(total,comma)
            L.append("| {r} | {q} | {d} | {l} | {v} | {p} |".format(
                r=i+1,q=esc(str(row[0])),d=esc(strip(row[2])),l=esc(link),v=esc(var),p=precio))
        L.append("")
    return "\n".join(L)+"\n"

def en_name(f): return f
def es_name(f): return f[:-4]+'.es.csv' if f.endswith('.csv') else f

open(os.path.join(HERE,'BOM_aliexpress.md'),'w',encoding='utf-8').write(build(0,en_name))
open(os.path.join(HERE,'BOM_aliexpress.es.md'),'w',encoding='utf-8').write(build(1,es_name))
print("OK: BOM_aliexpress.md y BOM_aliexpress.es.md")
