g++-4.1 -s sfk.cpp sfknet.cpp patch.cpp inst.cpp -o sfk-linux.exe
g++-3.3 -s sfk.cpp sfknet.cpp patch.cpp inst.cpp -DSFK_LIB5 -o sfk-linux-lib5.exe
cp sfk-linux.exe sfk

# to compile for Mac, use
# g++ -DMAC_OS_X sfk.cpp sfknet.cpp patch.cpp inst.cpp -o sfk

# generic compile, e.g. for 64-bit linux:
# g++ sfk.cpp sfknet.cpp patch.cpp inst.cpp -o sfk
