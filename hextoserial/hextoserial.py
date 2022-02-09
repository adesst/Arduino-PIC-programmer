#!./bin/python
import serial

'''
This is linux specific
'''
port_int = input("Enter PORT number: ")
port = '/dev/ttyACM%s' %(port_int)

try:
    ser = serial.Serial(port, 9600)
except:
    print("Couldn't open serial communication check the port name.")
    exit(1)

filename = input("Enter the full filename (full path): ")
try:
    hexFile = open(filename, "r")
except:
    print("Couldn't open the hex file. Make sure to enter the full path to the file.")
    ser.close()
    exit(1)

hexDict = {}

'''
reference to: https://microchipdeveloper.com/ipe:sqtp-hex-file-format

For example, consider the line:
:0300300002337A1E

Breaking this line into its components:

Record Length: 03 (3 bytes of data)
Address     : 0030 (the 3 bytes will be stored at 0030,
                    0031, and 0032)
Record Type : 00 (normal data)
Data        : 02, 33, 7A
Checksum    : 1E
'''
for line in hexFile:
    if int(line[3:7], 16) // 2 != 0x2007 and len(line) != 11:
        address = int(line[3:7], 16) // 2
        if address > 255:
            continue
        n = int(line[1:3], 16) // 2
        for i in range(n):
            # storing the word in the form of a tuple of two bytes
            hexDict[address] = (bytes.fromhex(line[9 + 4 * i:11 + 4 * i]), bytes.fromhex(line[11 + 4 * i:13 + 4 * i]))
            address += 1
    else:
        pass

# filling empty addresses in the middle with un-programmed word 0x3FFF
for i in range(max(hexDict.keys()) + 1):
    if i not in hexDict.keys():
        hexDict[i] = (b'\xff', b'\x3f')

def main():
    byte = b''
    i = 0
    decode = ''
    while True:
        if ser.inWaiting() > 0:
            byte = ser.read()
            if byte == b'\x01':
                # Send new word
                ser.write(hexDict[i][0])
                ser.write(hexDict[i][1])
                i += 1
            elif byte == b'\x02':
                # Send user input
                ser.write(input().encode("utf-8"))
            elif byte == b'\x03':
                # return to the first word
                i = 0
            elif byte == b'\x04':
                # Send the number of words in 2 bytes
                ser.write(bytes([len(hexDict.keys()) & 0b11111111, len(hexDict.keys()) >> 8]))
            else:
                # print received byte(character)
                print(byte.decode("ascii"), end="")


if __name__ == '__main__':
    try:
        main()
    except:
        ser.close()
        hexFile.close()
