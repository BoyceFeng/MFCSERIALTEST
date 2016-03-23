#pragma once
#include "stdafx.h"
using namespace std;

template < class T>
class QUEUE
{
public:
	QUEUE();//构造函数
	~QUEUE();//析构函数
	//初始化队列
	void Init_Queue(int dataLength);
	//判断队列是否为空
	bool Empty();
	//判断队列是否已满
	bool Full();
	//插入单个数据
	void Push_Elem(T t);
	//插入多个元素
	void Push_Elem(T *buf);
	//删除单个元素
	void Pop_Elem();
	//获取头部位置
	int Get_Front();
	//获取尾部位置
	int Get_rear();
	//获取队列长度
	int Get_Len();
	//void showenqueue();

private:
	T *data;
	int front;
	int rear;
	int length;
	bool isCreated;
};

