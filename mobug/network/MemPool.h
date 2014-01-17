#ifndef _MEMPOOL_H
#define _MEMPOOL_H

//ͷ�ļ�
#include <stdio.h>
#include <windows.h>
#include <malloc.h>
#include <process.h>

#define DEBUG 1

//�궨��
#define BLOCK 1024
#define MAXNUM 8

//���ݽṹ
struct ListNode
{
	void * ptr;
	struct ListNode * next;
};
ListNode lstMemRoot[MAXNUM];//δ����ڵ�����
ListNode lstNodeBuff;		//�ѷ���ڵ�����
CRITICAL_SECTION cs;		//�ٽ��

//��ʼ���ڴ��
void InitializeMemPool()
{
	InitializeCriticalSection(&cs);
	if(DEBUG) {
		//Log("��ʼ���ٽ��");
	}
	
	for(int i=0;i<MAXNUM;i++)
	{
		lstMemRoot[i].ptr = NULL;
		lstMemRoot[i].next = NULL;
	}

	lstNodeBuff.ptr = NULL;
	lstNodeBuff.next = NULL;
}

//�����ڴ�
void * MallocMem(int piMemLen)
{
	ListNode * tmp = NULL;
	void * ptr = NULL;

	int iChoose = 0; //ѡ������

	if(piMemLen <= 0){
		return 0;
	}
	else if (piMemLen > BLOCK * MAXNUM) {
		ptr = malloc(piMemLen+2); //ֱ�ӷ��䣬�����2���ֽ�
		*(char*)ptr = MAXNUM; //

		return (void*)((char*)ptr+2);
	}

	iChoose = (piMemLen-1)/BLOCK;

	if(lstMemRoot[iChoose].next == NULL)
	{
		//ֱ�ӷ����ڴ�
		ptr = malloc(piMemLen+2);	//�����2���ֽ�
		*((char*)ptr) = iChoose;

		return (void*)((char*)ptr+2);
	}
	else
	{
		//�����ٽ�����
		EnterCriticalSection(&cs);
		if(DEBUG) {
			//Log("�����ٽ��");
		}

		tmp = lstMemRoot[iChoose].next;

		//ɾ���˽ڵ�
		lstMemRoot[iChoose].next = tmp->next;

		//���˽ڵ���뵽�ڵ�����
		tmp->next = lstNodeBuff.next;
		lstNodeBuff.next = tmp;

		if(DEBUG) {
			//Log("�˳��ٽ��");
		}
		LeaveCriticalSection(&cs);

		return tmp->ptr;
	}

	return 0;
}

void FreeMem(void *pMemPtr)
{
	ListNode * tmp = NULL;

	int iChoose = 0; //ѡ������
	iChoose = *((char*)pMemPtr-2); //

	//
	if(iChoose > MAXNUM-1) {
		free((char*)pMemPtr-2);
		return;
	}

	//�����ٽ�����
	EnterCriticalSection(&cs);
	if(DEBUG) {
		//Log("�����ٽ��");
	}

	//����ѷ���ڵ�����Ϊ��
	if(lstNodeBuff.next == NULL)
	{
		tmp = (ListNode*)malloc(sizeof(ListNode));
		tmp -> ptr = NULL;
		tmp -> next = NULL;
	}
	else
	{
		tmp = lstNodeBuff.next;
		tmp -> ptr = NULL;
		tmp -> next = lstNodeBuff.next -> next;

		lstNodeBuff.next = tmp -> next;
	}

	//������յ��ڴ��ַ
	tmp -> next = lstMemRoot[iChoose].next;
	tmp -> ptr  = pMemPtr;
	lstMemRoot[iChoose].next = tmp;

	if(DEBUG) {
		//Log("�˳��ٽ��");
	}
	LeaveCriticalSection(&cs);
}

void CloseMemPool()
{
	DeleteCriticalSection(&cs);

	if(DEBUG) {
		//Log("�����ٽ��");
	}

	ListNode * pos = NULL;

	//�ͷ�����
	for(int i=0;i<MAXNUM;i++) {
		pos = lstMemRoot[i].next;
		while(pos != NULL) {
			if(pos->ptr != NULL) {
				free(pos->ptr);	//�ͷ��ڴ�
			}
			pos = pos->next;
		}
	}

	//�ͷ�����
	pos = lstNodeBuff.next;
	while(pos != NULL) {
		if(pos->ptr != NULL) {
			free(pos->ptr);	//�ͷ�ָ��
		}
		pos = pos->next;
	}

	//�ָ�ȫ�ֱ�����ʼֵ
	for(int i=0;i<MAXNUM;i++)
	{
		lstMemRoot[i].ptr = NULL;
		lstMemRoot[i].next = NULL;
	}
	lstNodeBuff.ptr = NULL;
	lstNodeBuff.next = NULL;
}
#endif

