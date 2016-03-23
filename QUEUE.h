#pragma once
#include "stdafx.h"
using namespace std;

template < class T>
class QUEUE
{
public:
	QUEUE();//���캯��
	~QUEUE();//��������
	//��ʼ������
	void Init_Queue(int dataLength);
	//�ж϶����Ƿ�Ϊ��
	bool Empty();
	//�ж϶����Ƿ�����
	bool Full();
	//���뵥������
	void Push_Elem(T t);
	//������Ԫ��
	void Push_Elem(T *buf);
	//ɾ������Ԫ��
	void Pop_Elem();
	//��ȡͷ��λ��
	int Get_Front();
	//��ȡβ��λ��
	int Get_rear();
	//��ȡ���г���
	int Get_Len();
	//void showenqueue();

private:
	T *data;
	int front;
	int rear;
	int length;
	bool isCreated;
};

