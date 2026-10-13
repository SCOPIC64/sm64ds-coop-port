"""Build an optional menu pack from the user's exact 980x240 DS UI sheet.

Requires Pillow. Borders are copied, not redrawn. Only label interiors are
filled from a clean column of the SAME button. No logos/copyright are copied.
"""
from pathlib import Path
import argparse, json, hashlib
from PIL import Image, ImageFilter, ImageDraw

def extract(source, output):
    sheet=Image.open(source).convert('RGBA')
    if sheet.size!=(980,240):raise ValueError('Expected the supplied 980x240 UI sheet')
    output.mkdir(parents=True,exist_ok=True)
    def frame(name,box,labels,clean_column,clean_row=None):
        im=sheet.crop(box)
        for left,top,right,bottom in labels:
            for y in range(top,bottom):
                sample=im.getpixel((clean_column,y))
                for x in range(left,right):
                    im.putpixel((x,y),im.getpixel((x,clean_row)) if clean_row is not None else sample)
        # White sprite-sheet background is a transparent key. Texture interiors
        # are beige/orange, so none of their pixels are discarded.
        for y in range(im.height):
            for x in range(im.width):
                if im.getpixel((x,y))[:3]==(255,255,255):im.putpixel((x,y),(0,0,0,0))
        im.save(output/name)
        return im
    panel=frame('panel.png',(271,2,383,99),[],0)
    selected=frame('selected.png',(0,0,241,34),[(26,6,218,28)],16)
    button=frame('button.png',(122,172,248,208),[(4,7,122,29)],7,6)

    # Each entry is a verified individual letter rectangle from the sheet.
    glyphs={}
    rows=[(58,75,'OPTIONS',[(2,16),(17,31),(32,45),(46,54),(55,69),(70,83),(84,98)]),
          (95,111,'DRAWAPICTURE!',[(3,16),(17,30),(31,44),(45,60),(69,82),(91,105),(106,114),(115,128),(129,142),(143,156),(157,170),(171,183),(185,190)]),
          (112,129,'TOUCHTHEPICTURE!',[(3,15),(16,28),(29,41),(42,54),(55,67),(76,88),(89,101),(102,113),(122,135),(137,144),(145,157),(158,170),(171,183),(184,196),(197,208),(210,216)]),
          (77,94,'FG',[(81,91),(65,75)])]
    for top,bottom,letters,ranges in rows:
        # DRAW line includes an extra A; the sequence above intentionally records it.
        if len(letters)!=len(ranges):raise ValueError((letters,len(ranges)))
        for char,(left,right) in zip(letters,ranges):
            if char in glyphs:continue
            crop=sheet.crop((left,top,right,bottom));mask=Image.new('L',crop.size)
            for y in range(crop.height):
                for x in range(crop.width):
                    r,g,b,_=crop.getpixel((x,y))
                    if (r,g,b)!=(255,255,255):mask.putpixel((x,y),255)
            bounds=mask.getbbox()
            if not bounds:continue
            crop=crop.crop(bounds);mask=mask.crop(bounds)
            glyph=Image.new('RGBA',(crop.width+2,crop.height+2))
            rim=Image.new('L',glyph.size);rim.paste(mask,(1,1));rim=rim.filter(ImageFilter.MaxFilter(3))
            glyph.paste((255,255,255,255),(0,0),rim)
            for y in range(crop.height):
                for x in range(crop.width):
                    r,g,b,_=crop.getpixel((x,y))
                    if (r,g,b)==(255,255,255):continue
                    glyph.putpixel((x+1,y+1),(0,0,0,255) if max(r,g,b)<150 else (255,0,255,255))
            if glyph.width>18 or glyph.height>18:raise ValueError((char,glyph.size))
            glyphs[char]=glyph
    # Complete the missing symbols with the same chunky outlined pixel style.
    # 5 has a straight top/left stroke; S uses an inset curved upper corner.
    patterns={
      'B':['11110','10001','10001','11110','10001','10001','11110'],
      'J':['00111','00010','00010','00010','10010','10010','01100'],
      'K':['10001','10010','10100','11000','10100','10010','10001'],
      'L':['10000','10000','10000','10000','10000','10000','11111'],
      'M':['10001','11011','10101','10101','10001','10001','10001'],
      'Q':['01110','10001','10001','10001','10101','10010','01101'],
      'V':['10001','10001','10001','10001','10001','01010','00100'],
      'X':['10001','10001','01010','00100','01010','10001','10001'],
      'Y':['10001','10001','01010','00100','00100','00100','00100'],
      'Z':['11111','00001','00010','00100','01000','10000','11111'],
      '0':['01110','10001','10011','10101','11001','10001','01110'],
      '1':['00100','01100','00100','00100','00100','00100','01110'],
      '2':['01110','10001','00001','00010','00100','01000','11111'],
      '3':['11110','00001','00001','01110','00001','00001','11110'],
      '4':['00010','00110','01010','10010','11111','00010','00010'],
      '5':['11111','10000','10000','11110','00001','00001','11110'],
      '6':['01110','10000','10000','11110','10001','10001','01110'],
      '7':['11111','00001','00010','00100','01000','01000','01000'],
      '8':['01110','10001','10001','01110','10001','10001','01110'],
      '9':['01110','10001','10001','01111','00001','00001','01110'],
      '?':['01110','10001','00001','00010','00100','00000','00100'],
      '.':['000','000','000','000','000','000','010'],
      ',':['000','000','000','000','000','010','100'],
      ':':['000','010','010','000','010','010','000'],
      '-':['00000','00000','00000','11111','00000','00000','00000'],
      '/':['00001','00001','00010','00100','01000','10000','10000'],
      '+':['00000','00100','00100','11111','00100','00100','00000'],
      '(' :['001','010','100','100','100','010','001'],
      ')' :['100','010','001','001','001','010','100'],
      '[' :['111','100','100','100','100','100','111'],
      ']' :['111','001','001','001','001','001','111'],
      '=' :['00000','00000','11111','00000','11111','00000','00000'],
      '_' :['00000','00000','00000','00000','00000','00000','11111']}
    extracted=''.join(sorted(glyphs))
    for char,rows in patterns.items():
        if char in glyphs:continue
        mask=Image.new('L',(len(rows[0])*2+4,18));draw=ImageDraw.Draw(mask)
        for y,row in enumerate(rows):
            for x,value in enumerate(row):
                if value=='1':draw.rectangle((x*2+2,y*2+2,x*2+3,y*2+3),fill=255)
        glyph=Image.new('RGBA',mask.size)
        glyph.paste((255,255,255,255),(0,0),mask.filter(ImageFilter.MaxFilter(5)))
        glyph.paste((0,0,0,255),(0,0),mask.filter(ImageFilter.MaxFilter(3)))
        glyph.paste((255,0,255,255),(0,0),mask);glyphs[char]=glyph
    atlas=Image.new('RGBA',(288,108))
    for value in range(32,127):
        char=chr(value);glyph=glyphs.get(char.upper(),glyphs['?'])
        if char==' ':continue
        cell=value-32;atlas.alpha_composite(glyph,(cell%16*18,cell//16*18))
    atlas.save(output/'font.png')
    (output/'pack.lua').write_text('sm64ds.menu_theme {\n panel="panel.png", button="button.png", selected="selected.png",\n font="font.png", border=4\n}\n',encoding='utf-8')
    (output/'provenance.json').write_text(json.dumps({'source_sha256':hashlib.sha256(Path(source).read_bytes()).hexdigest(),'sheet_size':[980,240],'copied_frames':{'panel':[271,2,383,99],'selected':[0,0,241,34],'button':[122,172,248,208]},'extracted_glyphs':extracted,'missing_glyphs':'Completed in matching outlined pixel style; lowercase uses uppercase shapes','labels':'Removed using clean material columns from the original frames'},indent=2))
    preview=Image.new('RGBA',(760,430),(24,29,38,255))
    def scaled(im,box):preview.alpha_composite(im.resize((box[2],box[3]),Image.Resampling.NEAREST),box[:2])
    scaled(panel,(16,16,400,395));scaled(selected,(440,90,296,66));scaled(button,(440,180,296,66));scaled(button,(440,270,296,66))
    # A source-asset proof, not a screenshot of the integrated game.
    preview.alpha_composite(atlas.resize((512,216),Image.Resampling.NEAREST),(30,30))
    preview.save(output/'asset-proof.png')
    print(json.dumps({'pack':str(output),'extracted_glyphs':extracted,'files':len(list(output.iterdir()))}))

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('sheet',type=Path);parser.add_argument('output',type=Path)
    args=parser.parse_args();extract(args.sheet,args.output)
