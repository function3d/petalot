#!/usr/bin/env python3
# Genera BOM_aliexpress.md (EN) y BOM_aliexpress.es.md (ES) desde los CSV de partes.
# Uso: python3 gen_bom_md.py
# Los precios/lotes/estado viven en los propios CSV (columnas price, lot, status).
import csv, re, math, os, datetime

HERE = os.path.dirname(os.path.abspath(__file__))

FILES = ['parts-list-petalot.csv','parts-list-electronics.csv','parts-list-cutter.csv','parts-list-tools.csv']
HEAD = {
 'parts-list-petalot.csv':   ('PETALOT machine',       'Máquina PETALOT'),
 'parts-list-electronics.csv':('Electronics for PETALOT','Electrónica para PETALOT'),
 'parts-list-cutter.csv':    ('Bottle Cutter',          'Cortador de botellas'),
 'parts-list-tools.csv':     ('Tools',                  'Herramientas'),
}
COLS = [['Qty','Component','Link','Price'],
        ['Cant.','Componente','Enlace','Precio']]
TOT_HEAD = ['Totals','Totales']
TOT_COLS = [['List','Total'],['Lista','Total']]
TOTAL_LBL = ['TOTAL (minimum, no shipping)','TOTAL (mínimo, sin envío)']
TOT_NOTE = None
NOTES = {
 'parts-list-petalot.csv': ['*Not included in the total: the wooden base for the PETALOT machine (cut to the dimensions in [this image](https://function3d.xyz/wp-content/uploads/2026/04/cotas2.jpg)).*',
                            '*No incluida en el total: la base de madera de la máquina PETALOT (cortada según las medidas de [esta imagen](https://function3d.xyz/wp-content/uploads/2026/04/cotas2.jpg)).*'],
 'parts-list-electronics.csv': ['*Not included in the total: the PCB (fabricated at JLCPCB).*',
                                '*No incluida en el total: la PCB (se fabrica en JLCPCB).*'],
 'parts-list-cutter.csv': ['*Not included in the total: the 48cm M6 threaded rod for the Bottle Cutter.*',
                           '*No incluida en el total: la varilla roscada 48cm M6 del cortador.*'],
}
PCB_PRODUCT = 'https://function3d.xyz/product/pcb-for-petalot'
WOOD_IMG = 'https://function3d.xyz/wp-content/uploads/2026/04/cotas2.jpg'

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
def price_lot(row):
    price = row[5].strip() if len(row)>5 else ''
    lot   = row[6].strip() if len(row)>6 else ''
    p = float(price) if re.match(r'^\d+(\.\d+)?$', price) else None
    l = int(lot) if lot.isdigit() else None
    return p, l
def line_total(qn, price, lot):
    if price is None: return None
    return (math.ceil(qn/lot)*price) if (isinstance(lot,int) and lot>0) else (price*qn)

def build(lang, csvname):
    comma = (lang==1)
    totals={}; grand=0.0; cache={}
    for f in FILES:
        rows=[r for r in csv.reader(open(os.path.join(HERE,csvname(f)),newline='',encoding='utf-8')) if r][1:]
        cache[f]=rows
        t=0.0
        for row in rows:
            p,l = price_lot(row)
            lt = line_total(qty_num(row[0]), p, l)
            if lt is not None: t+=lt
        totals[f]=t; grand+=t
    L=[]
    if lang==0:
        L.append(f"Prices updated: **{_today(0)}**. Indicative values, they may vary over time — check AliExpress for the current price.")
    else:
        L.append(f"Precios actualizados: **{_today(1)}**. Valores orientativos, pueden variar con el tiempo — consulta el precio actual en AliExpress.")
    L.append("")
    L.append(f"## {TOT_HEAD[lang]}"); L.append("")
    L.append("| "+" | ".join(TOT_COLS[lang])+" |"); L.append("|---|---|")
    for f in FILES: L.append(f"| {HEAD[f][lang]} | {eur(totals[f],comma)} |")
    L.append(f"| **{TOTAL_LBL[lang]}** | **{eur(grand,comma)}** |"); L.append("")
    for f in FILES:
        rows=cache[f]
        L.append(f"## {HEAD[f][lang]}"); L.append("")
        L.append("| "+" | ".join(COLS[lang])+" |")
        L.append("|"+"---|"*len(COLS[lang]))
        for i,row in enumerate(rows):
            p,l = price_lot(row)
            link = row[3]
            if f=='parts-list-electronics.csv' and i==0:
                link = 'https://jlcpcb.com/?from=FUNC · ' + PCB_PRODUCT
            desc = strip(row[2])
            if 'Wooden base' in row[2] or 'Base de madera' in row[2]:
                desc += ' <br><img src="' + WOOD_IMG + '" alt="Wooden base dimensions" width="380">'
            lt = line_total(qty_num(row[0]), p, l)
            precio = '' if lt is None else eur(lt,comma)
            L.append("| {q} | {d} | {l} | {p} |".format(
                q=esc(str(row[0])),d=esc(desc),l=esc(link),p=precio))
        L.append("")
        if f in NOTES:
            L.append(NOTES[f][lang]); L.append("")
    return "\n".join(L)+"\n"

def en_name(f): return f
def es_name(f): return f[:-4]+'.es.csv' if f.endswith('.csv') else f

open(os.path.join(HERE,'BOM_aliexpress.md'),'w',encoding='utf-8').write(build(0,en_name))
open(os.path.join(HERE,'BOM_aliexpress.es.md'),'w',encoding='utf-8').write(build(1,es_name))
print("OK: BOM_aliexpress.md y BOM_aliexpress.es.md")
