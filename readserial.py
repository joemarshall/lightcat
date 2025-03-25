import serial
import io

started=False
c=0
with serial.Serial('COM7',115200) as ser:
    wrapped=io.TextIOWrapper(ser)
    outfile=open(r"d:\temp\circle2.ppm","wb")
    while True:
        line=wrapped.readline()
        if line.find("P3")!=-1:
            line=line[line.find("P3"):]
            started=True
        if line.find("END")!=-1:
            started=False
            break
        if started:	
           print(c,line)
           outfile.write(line.encode())
           c+=1
