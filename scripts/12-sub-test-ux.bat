../sfk -nocol detab=3 testfiles/Formats/01-native-tab-crlf.txt >/dev/null
../sfk -nocol test $TCMD T01.1.detab testfiles/Formats/01-native-tab-crlf.txt

../sfk -nocol scantab testfiles/Formats >res-01.txt
../sfk -nocol test $TCMD T01.2.scantab res-01.txt

../sfk -nocol remcrlf testfiles/Formats/02-crlf.txt >/dev/null
../sfk -nocol test $TCMD T01.3.remcrlf testfiles/Formats/02-crlf.txt

../sfk -nocol addcrlf testfiles/Formats/04-lf.txt >/dev/null
../sfk -nocol test $TCMD T01.4.addcrlf testfiles/Formats/04-lf.txt

../sfk -nocol text-join-lines -quiet testfiles/Formats/05-split-text.txt res-02.txt
../sfk -nocol test $TCMD T01.5.joinlines res-02.txt

../sfk -nocol stat testfiles/FooBank >res-03.txt
../sfk -nocol test $TCMD T02.1.stat res-03.txt

../sfk -nocol list testfiles/FooBank >res-04.txt
../sfk -nocol test $TCMD T02.2.list res-04.txt

../sfk -nocol grep -pat include -dir testfiles >res-05.txt
../sfk -nocol test $TCMD T02.3.grep res-05.txt

../sfk -nocol grep -pat include -dir testfiles -file .cpp .hpp >res-06.txt
../sfk -nocol test $TCMD T02.4.grep res-06.txt

../sfk -nocol bin-to-src -pack testfiles/Formats/06-binary.jpg res-07.txt mydat
../sfk -nocol test $TCMD T02.5.bin2src res-07.txt

../sfk -nocol filter <testfiles/Formats/07-filter-src.txt -+something -:delete >res-08.txt
../sfk -nocol test $TCMD T02.6.filter res-08.txt

../sfk -nocol filter testfiles/Formats/07-filter-src.txt ++delete ++something >res-09.txt
../sfk -nocol test $TCMD T02.7.filter res-09.txt

../sfk -nocol addhead <testfiles/Formats/08-head-tail.txt >res-10.txt test1 test2 test3
../sfk -nocol test $TCMD T02.8.addhead res-10.txt

../sfk -nocol addtail -noblank <testfiles/Formats/08-head-tail.txt >res-11.txt test1 test2 test3
../sfk -nocol test $TCMD T02.9.addtail res-11.txt

../sfk -nocol deblank -yes -quiet testfiles/Formats
../sfk -nocol list testfiles >res-13.txt
../sfk -nocol test $TCMD T02.11.deblank res-13.txt

../sfk -nocol snapto=all-src.cpp -dir testfiles :testfiles/Formats -file .hpp .cpp >/dev/null
../sfk -nocol test $TCMD T03.1.snapto all-src.cpp

../sfk -nocol snapto=all-src-2.cpp -pure -dir testfiles :testfiles/Formats :testfiles/BaseLib -file .hpp .txt -norec >/dev/null
../sfk -nocol test $TCMD T03.2.snapto all-src-2.cpp

../sfk -nocol snapto=all-src-3.cpp -wrap=80 -prefix=#file# -stat -dir testfiles :testfiles/Formats :testfiles/BaseLib -file .hpp .cpp .txt >/dev/null
../sfk -nocol test $TCMD T03.3.snapto all-src-3.cpp

../sfk -nocol syncto=work.cpp -stop -dir testfiles :testfiles/Formats :testfiles/BaseLib -file .hpp .cpp >/dev/null
../sfk -nocol test $TCMD T03.4.syncto work.cpp

../sfk -nocol filter testfiles/Formats/10-dir-list.txt >res-14.txt -rep _/_/_ -rep xC:xx
../sfk -nocol test $TCMD T05.1.filtrep res-14.txt

../sfk -nocol filter testfiles/Formats/12-foo-jam.txt -ls+class -+void >res-15.txt
../sfk -nocol filter testfiles/Formats/12-foo-jam.txt +ls+class -+void >>res-15.txt
../sfk -nocol filter testfiles/Formats/12-foo-jam.txt ++class ++bar    >>res-15.txt
../sfk -nocol filter -case -lnum testfiles/Formats/12-foo-jam.txt -+bottle -+Trace >>res-15.txt
../sfk -nocol test $TCMD T05.2.filter res-15.txt

../sfk -nocol reflist -quiet -case -dir testfiles -file .hpp -dir testfiles -file .cpp >res-20-pre.txt
../sfk -nocol filter res-20-pre.txt >res-20.txt "-:00002 testfiles"
../sfk -nocol test $TCMD T06.1.reflist res-20.txt

../sfk -nocol remcrlf ../scripts/50-patch-all-src.cpp >/dev/null
../sfk -nocol patch -qs ../scripts/50-patch-all-src.cpp >/dev/null
../sfk -nocol test $TCMD T07.1.patch all-src.cpp

../sfk -nocol snapto=all-src-4.cpp testfiles/Formats 11-wide-line.txt 13-eof-null.txt 14-all-codes.txt >/dev/null
../sfk -nocol test $TCMD T08.1.snapform all-src-4.cpp

../sfk -nocol find testfiles/Formats scope01 quarter03 lead05 >res-21.txt
../sfk -nocol test $TCMD T09.1.findwrap res-21.txt

../sfk -nocol find testfiles/Formats the wrap >res-22.txt
../sfk -nocol test $TCMD T09.2.findwrap res-22.txt
