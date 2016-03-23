#include "QUEUE.h"
#include "stdafx.h"

//构造函数
template < class T>
QUEUE<T>::QUEUE()
{
	front = rear = 0;
	isCreated = false;
}

//析构函数
template < class T>
QUEUE<T>::~QUEUE()
{
	if (isCreated)
		delete []data;
}

//初始化数组
template < class T>
void QUEUE<T>::Init_Queue(int datalength)
{
	length = datalength;
	front = rear = 0;
	data = new T[length];
	isCreated = true;
}

//判断数组是否为空
template < class T>
bool QUEUE<T>::Empty()
{
	if (front == rear)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//判断数组是否已满
template < class T>
bool QUEUE<T>::Full()
{
	if ((rear + 1) % length == front)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//获得队列头部位置
template < class T>
int QUEUE<T>::Get_Front()
{
	return front;
}

//获取尾部位置
template < class T>
int QUEUE<T>::Get_rear()
{
	return rear;
}

//获取队列元素个数
template < class T>
int QUEUE<T>::Get_Len()
{
		return (rear - front + length) % length;
}

//往数组里添加单个元素
template < class T>
void QUEUE<T>::Push_Elem(T t)
{
	if (!Full())
	{
		data[rear] = t;
		rear = (rear + 1) % length;
	}
}

//往数组里添加n个元素
template < class T>
void QUEUE<T>::Push_Elem(T *buf)
{
	if (!Full())
	{
		for (int i = 0; i < sizeof(buf) / sizeof(T); i++)
		{
			data[rear] = buf[i];
			rear = (rear + 1) % length;
		}
	}
}

//删除单个数组元素
template < class T>
void QUEUE<T>::Pop_Elem()
{
	if (!Empty())
	{
		front = (front + 1) % length;
	}
}
