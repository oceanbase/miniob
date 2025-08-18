#include "pch.h"
#include "buffer_LRU.h"
#include "time.h"
#define CLOCKS_PER_SEC  ((clock_t)1000)
clock_t start, t_end; double duration;

void str2int(int& int_temp, const string& string_temp)
{
	stringstream stream(string_temp);
	stream >> int_temp;
}

vector<string> split(const string& str, const string& delim) {
	vector<string> res;
	if ("" == str) return res;
	char* strs = new char[str.length() + 1];
	strcpy(strs, str.c_str());

	char* d = new char[delim.length() + 1];
	strcpy(d, delim.c_str());

	char* p = strtok(strs, d);
	while (p) {
		string s = p;
		res.push_back(s);
		p = strtok(NULL, d);
	}
	delete strs, d;
	return res;
}

void CreateBlockWRTest(int blocks)
{
	FILE* fstream_w;
	char dbfile[] = "database.dbf";
	fstream_w = fopen(dbfile, "wb");
	if (fstream_w == NULL)
	{
		printf("open or create file failed!\n");
		exit(1);
	}
	else {
		//cout << "Open " << dbfile << " successfully!" << endl;
	}
	// 数据块号
	unsigned int block_num = 0;
	// 时间戳
	unsigned int btimestamp = 0;
	// 偏移量表
	unsigned short int offset[FRAMESIZE / RECORDSIZE];
	int BLOCK_HEAD = sizeof(btimestamp) + sizeof(offset);
	for (int i = 0; i < FRAMESIZE / RECORDSIZE; i++) { offset[i] = BLOCK_HEAD + i * RECORDSIZE; };

	for (int j = 0; j < blocks; j++)
	{
		// 写入块首部
		block_num = j;
		int writeCnt = fwrite(&block_num, sizeof(block_num), 1, fstream_w); // // 数据块号
		writeCnt = fwrite(&btimestamp, sizeof(btimestamp), 1, fstream_w);  // 时间戳
		writeCnt = fwrite(&offset, sizeof(offset), 1, fstream_w); // 块内偏移表
		// 写入记录
		char* p_schema = NULL;
		unsigned int timestamp = j;
		unsigned int length = 316;
		char namePtr[32];
		char adrressPtr[256];
		char genderPtr[4];
		char birthdayPtr[12];
		for (int i = 0; i < FRAMESIZE / RECORDSIZE; i++)
		{
			writeCnt = fwrite(&fstream_w, sizeof(fstream_w), 1, fstream_w);
			writeCnt = fwrite(&length, sizeof(length), 1, fstream_w);
			writeCnt = fwrite(&timestamp, sizeof(timestamp), 1, fstream_w);
			writeCnt = fwrite(&namePtr, sizeof(namePtr), 1, fstream_w);
			writeCnt = fwrite(&adrressPtr, sizeof(adrressPtr), 1, fstream_w);
			writeCnt = fwrite(&genderPtr, sizeof(genderPtr), 1, fstream_w);
			writeCnt = fwrite(&birthdayPtr, sizeof(birthdayPtr), 1, fstream_w);
		}
		char null[272]; //  填充字节使得4KB对齐
		writeCnt = fwrite(&null, sizeof(null), 1, fstream_w);
	}


	fclose(fstream_w);
}


void NoIndexQuery(int find_block_num)
{
	FILE* fstream_r;
	fstream_r = fopen("database.dbf", "rb");
	unsigned int blocknum = 0;
	cout << "query target: " << find_block_num << endl;
	int offset = find_block_num * FRAMESIZE;
	fseek(fstream_r, offset, SEEK_SET);
	int readCnt = fread(&blocknum, sizeof(blocknum), 1, fstream_r);
	cout << " query result: " << blocknum << endl;

	fclose(fstream_r);
}

void processTrace(int page_id, int operation, LRU* LRUCache) {
	if (operation == 0)
	{
#if VERBOSE
		cout << "---[trace 读操作] 读取page_id:" << page_id << " 的数据---" << endl;
#endif
		LRUCache->readData(page_id);
	}
	else if (operation == 1)
	{
#if VERBOSE
		cout << "---[trace 写操作] 修改page_id:" << page_id << " 的数据---" << endl;
#endif
		LRUCache->writeData(page_id, NULL);
	}
}


int main()
{
	cout << "*****************************************" << endl;
	cout << "*****************************************" << endl;
	cout << "\n";
	CreateBlockWRTest(50000);
	string tracefilename = "data-5w-50w-zipf.txt";
	ifstream traceFile(tracefilename.c_str());
	if (!traceFile)
	{
		cout << "Error opening " << tracefilename << " for input" << endl;
		exit(-1);
	}
	else {
		cout << "Open " << tracefilename << " successfully!" << endl;
	}
	char dbpath[] = "database.dbf";
	LRU LRU(BUFFSIZE, dbpath);
	int wr = 0;
	int page_id = 0;
	string trace;
	int request_num = 0;
	cout << "缓冲区大小:" << BUFFSIZE << endl;
	start = clock();
	while (!traceFile.eof())
	{
		getline(traceFile, trace);
		std::vector<string> res = split(trace, ",");
		str2int(wr, res[0]);
		str2int(page_id, res[1]);
		processTrace(page_id, wr, &LRU);
		request_num++;
	}
	t_end = clock();
	LRU.saveDirty2Disk();
	cout << "读写总数:" << request_num << endl;
	cout << "IO总数:" << LRU.getIOnum() << endl;
	cout << "命中总数:" << LRU.getHitNum() << endl;
	printf("命中率:%f%%\n", 100 * LRU.getHitNum() / float(request_num));
	duration = ((double)t_end - start) / CLOCKS_PER_SEC;
	printf("平均读写时间: %f ms\n", duration);
	system("pause");
	traceFile.close();
}
