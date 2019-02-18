COLLECTION = CollectionMain
EXPERIMENT = ExperimentMain
NWE_PATTERN_EXP = NewPatternExpMain
PARAMETERM = parameterM
NWE_PATTERN_PARAM = NewPatternParaM
M_ANALYSIS = parameterManalysis
SOURCE = ./source/
NEW_SOURCE = ./source/new/
DATA = ./data/NoService/
TOOL = include/tool/DateTime.o include/tool/StringToNumber.o include/tool/SubCharArray.o
GSPMINING = include/GSPMining/GSP.o
# 設定變數 之後用 $(xxx)即可以使用

# clean ".PHONY: clean" 表示 "clean" 不是一個真正的檔案目標，只是一個標記
.PHONY: clean cleanExp cleanNewPatExp cleanCol cleanM cleanNewM cleanMan

# ----- Experiment 預設跑實驗 ----------------------------------------------------------------
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

# -compile--
$(EXPERIMENT): $(EXPERIMENT).cpp $(TOOL) $(GSPMINING)
	g++ -o $@ $^ -O3 -std=c++11

# -clean--
cleanExp:
	rm -f $(EXPERIMENT)

# ----- for New Pattern, Experiment. ---------------------------------------------------------
new_u1: cleanNewPatExp $(NWE_PATTERN_EXP)
	./$(NWE_PATTERN_EXP) input=$(NEW_SOURCE) file=user1

new_u2: cleanNewPatExp $(NWE_PATTERN_EXP)
	./$(NWE_PATTERN_EXP) input=$(NEW_SOURCE) file=user2

new_u3: cleanNewPatExp $(NWE_PATTERN_EXP)
	./$(NWE_PATTERN_EXP) input=$(NEW_SOURCE) file=user3

new_u4: cleanNewPatExp $(NWE_PATTERN_EXP)
	./$(NWE_PATTERN_EXP) input=$(NEW_SOURCE) file=user4

new_u5: cleanNewPatExp $(NWE_PATTERN_EXP)
	./$(NWE_PATTERN_EXP) input=$(NEW_SOURCE) file=user5

# -compile--
$(NWE_PATTERN_EXP): $(NWE_PATTERN_EXP).cpp $(TOOL) $(GSPMINING)
	g++ -o $@ $^ -O3 -std=c++11

# -clean--
cleanNewPatExp:
	rm -f $(NWE_PATTERN_EXP)

# ----- collection 蒐集檔案 ------------------------------------------------------------------
col_my: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)my_0311_0729/ output=$(DATA)

col_u1: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)u1_0312_0513/ output=$(DATA)

col_u2: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)u2_0313_0425/ output=$(DATA)

col_u4: cleanCol $(COLLECTION)
	./$(COLLECTION) input=$(SOURCE)u4_0313_0730/ output=$(DATA)

# -compile--
$(COLLECTION): $(COLLECTION).cpp $(TOOL)
	g++ -o $@ $^ -O3 -std=c++11

# -clean--
cleanCol:
	rm -f $(COLLECTION)

# ----- Parameter M experiment ---------------------------------------------------------------
# M and M analysis
M: cleanM $(PARAMETERM)
#	./$(PARAMETERM) input=$(DATA) file=fb5e43235974561d 0 21 7

new_M: cleanNewM $(NWE_PATTERN_PARAM)
	./$(NWE_PATTERN_PARAM) input=$(NEW_SOURCE) file=user1 14 14 7

AnaM: cleanMan $(M_ANALYSIS)
	./$(M_ANALYSIS) input=./M/
# ./parameterManalysis input=./M/

# -compile--
$(PARAMETERM): $(PARAMETERM).cpp $(TOOL) $(GSPMINING)
	g++ -o $@ $^ -O3 -std=c++11

$(NWE_PATTERN_PARAM): $(NWE_PATTERN_PARAM).cpp $(TOOL) $(GSPMINING)
	g++ -o $@ $^ -O3 -std=c++11

$(M_ANALYSIS): $(M_ANALYSIS).cpp
	g++ -o $@ $^ -O3 -std=c++11

# -clean
cleanM:
	rm -f $(PARAMETERM)

cleanNewM:
	rm -f $(NWE_PATTERN_PARAM)

cleanMan:
	rm -f $(M_ANALYSIS)

# ----- other --------------------------------------------------------------------------------
clean: cleanExp cleanNewPatExp cleanCol cleanM cleanNewM cleanMan

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

