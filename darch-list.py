import os
import sys

def HexUpper(integer):
	return hex(integer).upper().replace('0X', '0x')

ARCHIVE_INT_SIZE = 8
MAGIC_NO = 627455

if len(sys.argv) != 2:
	print('Usage: ' + sys.argv[0] + ' <archive name>.')
	quit()

archive = sys.argv[1]

f = open(archive, 'rb')
magic = int.from_bytes(f.read(ARCHIVE_INT_SIZE), byteorder = 'little', signed = True) # Consume magic
print('Archive size: ' + HexUpper(os.path.getsize(archive)) + '.')
print('Magic number: ' + HexUpper(magic) + '.')

if magic != MAGIC_NO:
	print('Bad magic number.')
else:
	entryCount = int.from_bytes(f.read(ARCHIVE_INT_SIZE), byteorder = 'little', signed = True)
	print('Entry count: ' + HexUpper(entryCount) + '.')
	for i in range(0, entryCount):
		magic = int.from_bytes(f.read(ARCHIVE_INT_SIZE), byteorder = 'little', signed = True)
		if magic != MAGIC_NO:
			print('Bad magic number at idx ' + HexUpper(i) + '.')
			break

		pathLen = int.from_bytes(f.read(ARCHIVE_INT_SIZE), byteorder = 'little', signed = True)
		relativePath = f.read(pathLen).decode('ascii')
		f.seek(8, 1) # Skip permissions
		size = int.from_bytes(f.read(ARCHIVE_INT_SIZE), byteorder = 'little', signed = True)
		f.seek(size, 1) # Skip the actual body because it's not relevant
		print('Entry ' + HexUpper(i) + ': ')
		print('\tRelative Path: ' + relativePath + '.')
		print('\tFile Size: ' + HexUpper(size) + '.')

f.close()