import sys
sys.path.insert(0, sys.argv[3])
from pdfminer.high_level import extract_text
t = extract_text(sys.argv[1])
open(sys.argv[2], 'w').write(t)
print(len(t))
