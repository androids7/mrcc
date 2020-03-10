#ifndef __LITTLEC_H__
#define __LITTLEC_H__

#define LC_EXIT 0x01       //�¼���Ӧ״̬Ϊ�˳�




extern char lc_state;      //�����¼���Ӧ�������״̬�ж�,����ǰ���ֶ���Ϊ0
extern int32 timer[2];     //���н�����Ҫ�ֶ�ֹͣ��������ʱ��





#define NUM_FUNC           50
#define NUM_GLOBAL_VARS    60
#define NUM_LOCAL_VARS     60
#define ID_LEN             21
#define NUM_PARAMS         20        //�û���������������
#define PROG_SIZE          30601     //������󳤶�Ϊ30K
#define RESULT_SIZE        5000

/* ����LITTLEC���� */
int StartLittleC(char *mem);

//��func��ִ�г���ǰ���Ǳ���ʹ��StartLittleCִ�й�
int StartLittleCFunc(char *func);

//ִ��event����
int StartLittleCEvent(int code, int p1, int p2);

/* ��ʼ��LITTLEC�������ڴ� */
int32 InitLittleC(void);

/* �ͷ�LITTLEC�������ڴ� */
void ReleaseLittleC(void);

/* ��ȡ���к�����ֽ�����мǲ����ͷţ������� */
char* GetResult(void);

//�ⲿ�˳�LITTLEC(����ֹͣ��ʱ��)
int littleCStop(void);


#endif /* #ifndef __LITTLEC_H__ */