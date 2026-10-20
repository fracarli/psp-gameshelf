#!/usr/bin/env python3
"""Render the original bridge icon geometry used by assets/ICON0.svg to PSP PNG."""
from pathlib import Path
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
SCALE=8
im=Image.new('RGBA',(80*SCALE,80*SCALE),(0,0,0,0));d=ImageDraw.Draw(im)
def box(coords,radius,color):
    d.rounded_rectangle(tuple(round(v*SCALE) for v in coords),radius=radius*SCALE,fill=color)
def line(points,width,color):
    p=[(round(x*SCALE),round(y*SCALE)) for x,y in points]
    d.line(p,fill=color,width=width*SCALE,joint='curve')
    r=width*SCALE/2
    for x,y in [p[0],p[-1]]:d.ellipse((round(x-r),round(y-r),round(x+r),round(y+r)),fill=color)
navy='#101c30';mint='#71e0ce';white='#eaf4ff';amber='#ffc978'
box((2,2,78,78),18,navy)
line([(16,49),(64,49)],6,mint)
line([(21,49),(21,36)],6,mint);line([(59,49),(59,36)],6,mint)
line([(21+38*t/100,36-54*(t/100)*(1-t/100)) for t in range(101)],6,mint)
for x,y in [(29,29),(40,25),(51,29)]:line([(x,y),(x,48)],3,mint)
box((10,49,28,62),4,white);box((52,49,70,62),4,amber)
d.ellipse((17*SCALE,53*SCALE,21*SCALE,57*SCALE),fill=navy)
line([(57,55),(65,55)],2,navy)
im.resize((80,80),Image.Resampling.LANCZOS).save(ROOT/'assets/ICON0.PNG')
print('Rendered the original square bridge icon (80x80 RGBA).')
