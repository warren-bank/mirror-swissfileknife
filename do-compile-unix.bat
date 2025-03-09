g++ -s sfk.cpp sfknet.cpp patch.cpp inst.cpp -o sfk

# to compile SFK for old linux systems with lib5:
# g++-3.3 -s sfk.cpp sfknet.cpp patch.cpp inst.cpp -DSFK_LIB5 -o sfk-linux-lib5.exe

# to compile for Mac, use
# g++ -DMAC_OS_X sfk.cpp sfknet.cpp patch.cpp inst.cpp -o sfk
