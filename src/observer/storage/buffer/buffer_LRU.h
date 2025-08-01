#include "stdio.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>

#define VERBOSE 0
#define RECORDSIZE 316
#define FRAMESIZE 4096 
#define BUFFSIZE 1024 // 可修改缓冲区大小
#define MAXSIZE  FRAMESIZE*BUFFSIZE

using namespace std;

struct bFrame
{
	char field[FRAMESIZE];
};

struct bBuff
{
	bFrame frames[BUFFSIZE];
};



struct Record
{
	int page_id;
	int slot_num;
};

struct Frame
{
	int frame_id;
	int offset;
};


class DiskSpaceManager {
private:
	FILE* fstream;
public:
	DiskSpaceManager() :fstream(NULL) {}

	void openFile(char filepath[]) {
		fstream = fopen(filepath, "rb+");
	}

	void readDisk(int page_id, int frame_id, bBuff* buff) {
		bFrame bframe;
		unsigned int block_num = 0;
		fseek(fstream, page_id * FRAMESIZE, SEEK_SET);
		if (fread(&bframe.field, sizeof(bframe.field), 1, fstream) == 1) {
#if VERBOSE
			cout << "读取 page_id:" << page_id << " 到 frame_id:" << frame_id << " ... 缓存成功。" << endl;
#endif
		}
		else {
#if VERBOSE
			cout << "读取 page_id:" << page_id << " 到 frame_id:" << frame_id << " ... 缓存失败！" << endl;
#endif
		}
		buff->frames[frame_id] = bframe;
	}

	void writeDisk(int page_id, int frame_id, bBuff* buff) {
		bFrame bframe = buff->frames[frame_id];
		fseek(fstream, page_id * FRAMESIZE, SEEK_SET);
		if (fwrite(&bframe.field, sizeof(bframe.field), 1, fstream) == 1) {
#if VERBOSE
			cout << "写入 frame_id:" << frame_id << " 到 page_id:" << page_id << " 磁盘成功。" << endl;
#endif
		}
		else {
#if VERBOSE
			cout << "写入 frame_id:" << frame_id << " 到 page_id:" << page_id << " 磁盘失败！" << endl;
#endif
		}
	}
};

//双向链表+哈希表实现LRU
struct BufferControlBlocks
{
	int page_id, frame_id, ref, count, time, dirty;
	BufferControlBlocks* _prev;
	BufferControlBlocks* _next;
	BufferControlBlocks() : _next(NULL), _prev(NULL) {}
	BufferControlBlocks(int page_id, int frame_id) : page_id(page_id), frame_id(frame_id), count(0), time(0), dirty(0) { BufferControlBlocks(); }
	void disconnect() {
		if (_prev) _prev->_next = _next;
		if (_next) _next->_prev = _prev;
	}
	void Insert(BufferControlBlocks* node) {
		if (_next) {
			node->_prev = this; // this
			_next->_prev = node;
			node->_next = _next;
			_next = node;
		}
	}
};
// 储存frames和pages的元数据Buffer Control Blocks
class LRU {
private:
	int size, capacity; //当前大小，容量上限
	unordered_map<int, BufferControlBlocks*> hashmap;
	stack<int> frame_ids;
	BufferControlBlocks* head, * tail;
	int hitNum; // 命中数
	int IONum; // 磁盘IO
	bBuff* buff;
	int f2p[BUFFSIZE]; // frame_id到page_id的映射
	DiskSpaceManager diskSpaceManager;
public:
	LRU(int capacity, char dbpath[]) : size(0), capacity(capacity) {
		head = new BufferControlBlocks;
		tail = new BufferControlBlocks;
		head->_next = tail;
		tail->_prev = head;
		hitNum = IONum = 0;
		buff = new bBuff;
		diskSpaceManager.openFile(dbpath);
		for (int i = 0; i < BUFFSIZE; i++)f2p[i] = 0;
		for (int i = 0; i < capacity; i++)frame_ids.push(i);
	}
	int read(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
#if VERBOSE
			cout << "该page_id: " << page_id << " 未存在于缓存中！" << endl;
#endif
			return -1;
		}
		BufferControlBlocks* node = hashmap[page_id];
		node->disconnect();
		head->Insert(node);
		int fid = node->frame_id;
		return fid;
	}

	int read_id(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
#if VERBOSE
			cout << "该page_id: " << page_id << " 未存在于缓存中！";
#endif
			return -1;
		}
		BufferControlBlocks* node = hashmap[page_id];
		return node->frame_id;
	}

	int put(int page_id, int frame_id = 0) {
		BufferControlBlocks* node;
		if (page_id == -1) { cout << "申请磁盘新page" << endl; };
		if (hashmap.find(page_id) == hashmap.end()) {
			if (size < capacity) {
				frame_id = frame_ids.top();
				frame_ids.pop();
				node = new BufferControlBlocks(page_id, frame_id);
				size++;
			}
			else
			{
				// 置换
				node = tail->_prev;
				int i = 0;
				for (; node->ref > 1 || node->count > 1; node = node->_next)
				{
					node->ref = 0; i++; if (i == capacity - 1) { cout << "警告：缓存溢出！进行强行置换"; };
					break;
				};
				// 脏数据写入磁盘
				if (node->dirty)
				{
					node->dirty = 0;
					diskSpaceManager.writeDisk(node->page_id, node->frame_id, buff);
#if VERBOSE
					cout << "缓存区满，发生置换，已将page_id:" << node->page_id << " 数据块写入磁盘！" << endl;
#endif
				}
				frame_id = node->frame_id;
				node->disconnect();
				hashmap.erase(node->page_id);
				node->page_id = page_id;
				node->frame_id = frame_id;
			}
			hashmap[page_id] = node;
			head->Insert(node);
		}
		else {
			node = hashmap[page_id];
			node->disconnect();
			head->Insert(node);
			frame_id = node->frame_id;
		}
		return frame_id;
	}

	int selectVictim() {
		BufferControlBlocks* node;
		node = tail->_prev;
		int i = 0;
		for (; node->ref > 1 || node->count > 1; node = node->_next)
		{
			node->ref = 0; i++; if (i == capacity) { cout << "缓存溢出！即将进行强行置换"; };
		};
		return node->frame_id;
	}

	void remove(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			cout << "错误！page_id: " << page_id << " 不存在缓存中！" << endl;
		}

		BufferControlBlocks* node = hashmap[page_id];
		frame_ids.push(node->frame_id);
		node->disconnect();
		hashmap.erase(node->page_id);
		size--;
	}

	int setDirty(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BufferControlBlocks* node = hashmap[page_id];
		node->dirty = 1;
		node->time += 1;
		node->disconnect(); // �����ײ�
		head->Insert(node);
		return node->frame_id;
	}

	int setCount(int page_id) {
		if (hashmap.find(page_id) == hashmap.end()) {
			return -1;
		}
		BufferControlBlocks* node = hashmap[page_id];
		node->count += 1;
		node->time += 1;
		node->disconnect();
		head->Insert(node);
		return node->frame_id;
	}

	unsigned int readData(int page_id) {
		int frame_id = read(page_id);
		if (frame_id == -1)
		{

			IONum++;
			frame_id = put(page_id);
			diskSpaceManager.readDisk(page_id, frame_id, buff);
		}
		else {

			hitNum++;
		}
#if VERBOSE
		unsigned int* bln_ptr;
		char bln[4];
		for (int i = 0; i < 4; i++)
		{
			bln[i] = buff->frames[frame_id].field[i];
		}
		bln_ptr = (unsigned int*)&bln;
		cout << "读到的数据（块号）:" << *bln_ptr << " （指针指向块头，读取长度为4bit，记录着该块在整个.dbf文件的块号）" << endl;
		if (*bln_ptr == page_id) cout << "读取数据验证通过!" << endl;
#endif
		return 1;
	}

	int writeData(int page_id, char* data) {
		int frame_id = read(page_id);
		if (frame_id == -1)
		{

			IONum++;
			frame_id = put(page_id);
			diskSpaceManager.readDisk(page_id, frame_id, buff);
		}
		else {

			hitNum++;
		}
		buff->frames[frame_id].field[1]++;
		BufferControlBlocks* node = hashmap[page_id];
		node->dirty = 1;
#if VERBOSE
		unsigned int* timestamp;
		char ts[4];
		for (int i = 0; i < 4; i++)
		{
			ts[i] = buff->frames[frame_id].field[4 + i];
		}
		timestamp = (unsigned int*)&ts;
		cout << "即将写入的数据（时间戳）:" << *timestamp << " （指针指向距离块头4bit位置，写入长度为4bit，记录着该块的时间戳）" << endl;;
#endif
		return 1;
	}

	int getIOnum() {
		return IONum;
	}

	int getHitNum() {
		return hitNum;
	}

	void saveDirty2Disk() {
		for (BufferControlBlocks* node = head->_next; node != tail; node = node->_next)
		{
			if (node->dirty == 1)
			{
				diskSpaceManager.writeDisk(node->page_id, node->frame_id, buff);
				IONum++;
				node->dirty = 0;
			}
		}
		delete tail, head, buff;
#if VERBOSE
		cout << "即将关闭LRUCache: 已将全部脏数据写进磁盘!" << endl;
#endif
	}
};
