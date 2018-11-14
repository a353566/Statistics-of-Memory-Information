TARGETS = Main
TOOL = include/tool/DateTime.o include/tool/StringToNumber.o
GSPMINING = include/GSPMining/GSP.o
# 設定變數 之後用 $(xxx)即可以使用

my: all
	./$(TARGETS) ./data/my_0311_0530/

u2: all
	./$(TARGETS) ./data/u2_0313_0324/
	
u4: all
	./$(TARGETS) ./data/u4_0313_0404/

all: clean $(TARGETS)
# ./xxx 是順便執行

.PHONY: clean
# 表示 "clean" 不是一個真正的檔案目標，只是一個標記
clean:
	rm -f $(TARGETS)

$(TARGETS): $(TARGETS).cpp $(TOOL) $(GSPMINING)
	g++ -o $@ $^ -O3 -std=c++11
#	g++ -o $@ -I$(RNNLIB) $^ $(libnetcdf_c++.so)
# -I/aaa/bbb 以"/aaa/bbb"當lib的目錄
# -std=c++11   -> for "auto"

%.o: %.cpp
	g++ -o $@ -c $^ -O3 -std=c++11

%: %.cpp
	g++ -o $@ $^ -std=c++11
# -o xxx 表示以 xxx 當輸出檔名
# out: in1.c in2.c
# $@ 表示"目標檔名" (out)
# $< 表示"需求檔的第一個" (in1.c)
# $^ 表示"所有需求檔" (in1.c in2.c)

