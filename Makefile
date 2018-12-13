COLLECTION = CollectionMain
EXPERIMENT = ExperimentMain
PARAMETERM = parameterM
M_ANALYSIS = parameterManalysis
SOURCE = ./source/
DATA = ./data/
TOOL = include/tool/DateTime.o include/tool/StringToNumber.o include/tool/SubCharArray.o
GSPMINING = include/GSPMining/GSP.o
# 設定變數 之後用 $(xxx)即可以使用

# clean ".PHONY: clean" 表示 "clean" 不是一個真正的檔案目標，只是一個標記
.PHONY: clean cleanExp cleanCol cleanM cleanMan

clean: cleanExp cleanCol cleanM cleanMan
#	rm -f $(EXPERIMENT) $(COLLECTION) $(PARAMETERM) $(M_ANALYSIS)

# ----- Experiment 預設跑實驗
# ./xxx 是順便執行
my: cleanExp $(EXPERIMENT)
	./$(EXPERIMENT) input=$(DATA) file=fb5e43235974561d
#	./$(EXPERIMENT) input=$(DATA) file=fb5e43235974561d_0311_0729
#	./$(EXPERIMENT) input=$(DATA) file=fb5e43235974561d_0311_0530

u1: cleanExp $(EXPERIMENT)
	./$(EXPERIMENT) input=$(DATA) file=851aa1db59aad4fd

u2: cleanExp $(EXPERIMENT)
	./$(EXPERIMENT) input=$(DATA) file=143609e134ebdd90
#	./$(EXPERIMENT) input=$(DATA) file=143609e134ebdd90_0313_0425
#	./$(EXPERIMENT) input=$(DATA) file=143609e134ebdd90_0328_0413
#	./$(EXPERIMENT) input=$(DATA) file=143609e134ebdd90_0313_0324

u4: cleanExp $(EXPERIMENT)
	./$(EXPERIMENT) input=$(DATA) file=b05216f6d3a29ae9

# -clean--
cleanExp:
	rm -f $(EXPERIMENT)

# ----- collection 蒐集檔案
col_my: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)my_0311_0729/ output=$(DATA)

col_u1: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)u1_0312_0513/ output=$(DATA)

col_u2: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)u2_0313_0425/ output=$(DATA)

col_u4: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)u4_0313_0730/ output=$(DATA)

# -clean--
cleanCol:
	rm -f $(COLLECTION)

# ----- Parameter M experiment
M: $(PARAMETERM)
#	./$(PARAMETERM) input=$(DATA) file=fb5e43235974561d 0 21 7

cleanM:
	rm -f $(PARAMETERM)

AnaM: $(M_ANALYSIS)
	./$(M_ANALYSIS) input=./M/
# ./parameterManalysis input=./M/

cleanMan:
	rm -f $(M_ANALYSIS)

# ----- other
$(EXPERIMENT): $(EXPERIMENT).cpp $(TOOL) $(GSPMINING)
	g++ -o $@ $^ -O3 -std=c++11

$(COLLECTION): $(COLLECTION).cpp $(TOOL)
	g++ -o $@ $^ -O3 -std=c++11

$(PARAMETERM): $(PARAMETERM).cpp $(TOOL) $(GSPMINING)
	g++ -o $@ $^ -O3 -std=c++11

$(M_ANALYSIS): $(M_ANALYSIS).cpp
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

