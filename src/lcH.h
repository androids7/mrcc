#include "mrc_base.h"

#define local static

#define iswhite(c) ((c) == ' ' || (c) == '\t')
#define isalpha(c) (((c)>='a'&&(c)<='z')||((c)>='A'&&(c)<='Z')||(c)=='_')
#define isdigit(c) ((c)>='0'&&(c)<='9')
#define toupper(c) ((c)|=0x20)//ת��Ϊ��д��ĸ
#define tolower(c) ((c)&=0xdf)//ת��ΪСд��ĸ


enum tok_types {
    DELIMITER = 1,
    IDENTIFIER,
    NUMBER,
    KEYWORD,
    STRING,
    BLOCK
};

/* ���������ӹؼ��� */
enum tokens {
    ARG=1, 
    CHAR,
    INT,
    IF,
    ELSE, 
    FOR, 
    DO,
    WHILE,
    CONTINUE,
    SWITCH,
    RETURN,
    EOL,
    FINISHED,
    END
};

//�ڲ�������
enum func {
    FUNC_POINT,
    FUNC_REFRESH,
    FUNC_LINE,
    FUNC_RECT,
    FUNC_DRAWTXT,
    FUNC_DRAWTXT2,
    FUNC_DRAWBMP,
    FUNC_CLS,
    FUNC_SLEEP,
    FUNC_SRECT,
    FUNC_EFFSETCON,
    FUNC_IMG,
    FUNC_RAND,
    FUNC_STRW,
    FUNC_STRH,
    FUNC_TIMER,
    FUNC_TIMERSTOP,
    FUNC_IPRINT,
    FUNC_PUTCH,
    FUNC_PRINT,
    FUNC_STOPSOUND,
    FUNC_PLAYSOUND,
    FUNC_SMS,
    FUNC_PRINTSCREEN,
    FUNC_RUNMRP,
    FUNC_MKDIR,
    FUNC_REMOVE,
    FUNC_OPEN,
    FUNC_CLOSE,
    FUNC_READ,
    FUNC_SEEK,
    FUNC_WRITE,
    FUNC_GETLEN,
    FUNC_RENAME,
    FUNC_EXIT,
    FUNC_END    //����
};

enum error_msg {
    SYNTAX,
    NOT_VAR,
    SEMI_EXPECTED,
    FUNC_UNDEF,
    TYPE_EXPECTED,
    RET_NOCALL,
    PAREN_EXPECTED,
    WHILE_EXPECTED,
    NEST_FUNC,
    TOO_MANY_VARS,
    TOO_MANY_FUNC,
    DIV_BY_ZERO,
    ERROR_END    //����
};

/* �������������(���� ->) */
enum double_ops {
    LT=1,
    LE, 
    GT, 
    GE,
    EQ,
    NE
};

/* ȫ�ֱ���*/
typedef struct var_type {
  char var_name[ID_LEN];
  int value;
}var_type;


typedef struct func_type{
  char func_name[ID_LEN];
  char *loc;  /* �ļ���ڵ�λ�� */
}func_type;

typedef struct intern_func_type{
  char *f_name; /* �������� */
  int (*p)();   /* ����ָ�� */
}intern_func_type;


struct commands { /* �ؼ��ֲ��ұ� */
  char command[7];
  char tok;
}const table[] = { /* ���������Сд */
  "if", IF,
  "else", ELSE,
  "for", FOR,
  "do", DO,
  "while", WHILE,
  "int", INT,
  "return", RETURN,
  "", END  /* ������� */
};

#define CONSTANT 25
struct {
  char name[8];
  int value;
}const constant[CONSTANT]={
    "KY_DOWN",MR_KEY_PRESS,
    "KY_UP",MR_KEY_RELEASE,
    "MS_DOWN",MR_MOUSE_DOWN,
    "MS_UP",MR_MOUSE_UP,
    "MS_MOVE",MR_MOUSE_MOVE,
    "_0",MR_KEY_0,               //���� 0
    "_1",MR_KEY_1,               //���� 1
    "_2",MR_KEY_2,               //���� 2
    "_3",MR_KEY_3,               //���� 3
    "_4",MR_KEY_4,               //���� 4
    "_5",MR_KEY_5,               //���� 5
    "_6",MR_KEY_6,               //���� 6
    "_7",MR_KEY_7,               //���� 7
    "_8",MR_KEY_8,               //���� 8
    "_9",MR_KEY_9,               //���� 9
    "_STAR",MR_KEY_STAR,         //���� *
    "_POUND",MR_KEY_POUND,       //���� #
    "_UP",MR_KEY_UP,             //���� ��
    "_DOWN",MR_KEY_DOWN,         //���� ��
    "_LEFT",MR_KEY_LEFT,         //���� ��
    "_RIGHT",MR_KEY_RIGHT,       //���� ��
    "_SLEFT",MR_KEY_SOFTLEFT,    //���� �����
    "_SRIGHT",MR_KEY_SOFTRIGHT,  //���� �����
    "_SELECT",MR_KEY_SELECT,     //���� ȷ��/ѡ��
    "",END
};
/* ���ط��ŵ������� */
#define putback()                \
do{                              \
    char *t;                     \
                                 \
    if(!g_error)                 \
    {   t = token;               \
        for(; *t; t++) prog--;   \
    }                            \
}while(0)