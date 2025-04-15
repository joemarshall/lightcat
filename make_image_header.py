import subprocess,sys
from pathlib import Path

Import("env")
try:
    import png,lz4
except:    
    env.Execute("$PYTHONEXE -m pip install pypng lz4")

for x in Path("images").glob("*.png"):
    outFile=Path("src\\images") / (x.stem+".c")
    if not outFile.exists() or x.stat().st_mtime> outFile.stat().st_mtime:
        if x.name.startswith("wheel") or x.name.startswith("standby"):
            # background image - no transparency
            format="RGB565"
        else:
            format="RGB565A8"
        args=[sys.executable,"LVGLImage.py",x.absolute(),"--ofmt","C","--cf",format,"-o","src\\images"]
        print(args)
        subprocess.check_call(args)
    else:
        print(f"{outFile} is up to date")

