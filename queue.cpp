#include "QUEUE.h"
#include "stdafx.h"

//���캯��
template < class T>
QUEUE<T>::QUEUE()
{
	front = rear = 0;
	isCreated = false;
}

//��������
template < class T>
QUEUE<T>::~QUEUE()
{
	if (isCreated)
		delete []data;
}

//��ʼ������
template < class T>
void QUEUE<T>::Init_Queue(int datalength)
{
	length = datalength;
	front = rear = 0;
	data = new T[length];
	isCreated = true;
}

//�ж������Ƿ�Ϊ��
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

//�ж������Ƿ�����
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

//��ö���ͷ��λ��
template < class T>
int QUEUE<T>::Get_Front()
{
	return front;
}

//��ȡβ��λ��
template < class T>
int QUEUE<T>::Get_rear()
{
	return rear;
}

//��ȡ����Ԫ�ظ���
template < class T>
int QUEUE<T>::Get_Len()
{
		return (rear - front + length) % length;
}

//����������ӵ���Ԫ��
template < class T>
void QUEUE<T>::Push_Elem(T t)
{
	if (!Full())
	{
		data[rear] = t;
		rear = (rear + 1) % length;
	}
}

//�����������n��Ԫ��
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

//ɾ����������Ԫ��
template < class T>
void QUEUE<T>::Pop_Elem()
{
	if (!Empty())
	{
		front = (front + 1) % length;
	}
}
