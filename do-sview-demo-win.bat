@rem this simply collects all files from testfiles
@rem and then loads the collection in sfk snapview.
sfk snapto=all-src.cpp -dir testfiles -file .hpp .cpp
sview all-src.cpp
