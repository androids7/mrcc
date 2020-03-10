/* A Little C interpreter. */
#include "mrc_base.h"
#include "momo.h"
#include "smp.h"

#include "littlec.h"
#include "lcH.h"
#include "lcDefine.h"

char tmp[514]; //�ɹ���ʹ��
local char g_error;
local char *prog;    /* Դ�뵱ǰλ��*/
local char *p_buf;   /* ָ��Դ����ʼλ�� */
local char token[80];
local char token_type; 
local char tok;
local int resultTxtlen;
local int functos;        /* �������ö�ջλ�� */
local int func_index;     /* �ں������е�λ�� *///�Ѿ�û����
local int gvar_index;     /* ��ȫ�ֱ������е�λ�� */
local int lvartos;        /* �ھֲ�������ջ�е�λ�� */
local int ret_value;      /* ��������ֵ */
local char *ResultString;
local char *e[ERROR_END];
local const char g_relops[7] = {
    LT, LE, GT, GE, EQ, NE, 0
};


local int           call_stack[NUM_FUNC+1];
var_type            local_var_stack[NUM_LOCAL_VARS+1];//�û�����ı������͡����������������ͺ�ֵ
var_type            global_vars[NUM_GLOBAL_VARS+1];//ͬ��
func_type           func_table[NUM_FUNC+1];//�û��������ͣ�����������������ֵ���ͺ����λ��
intern_func_type    intern_func[FUNC_END+1];//�ڲ��������ͣ��������������ڲ���������ڣ�����ֵͳһΪINT��
#include "lcCall.h"

//////////////////////////////////////////////////////////////////////////////////////
/* ����һ������ �� ����� */
local void interp_block(void)
{
    int value;
    char block = 0;

    do {
        if(g_error) return;
        token_type = get_token();

        /* �������һ������, ���ص�һ��; */
        if(token_type == IDENTIFIER)
        {
            /* ���ǹؼ��֣���ô������ʽ */
            putback();
            eval_exp(&value);  /* ������ʽ */
            if(*token!=';') sntx_err(SEMI_EXPECTED);
        }
        else if(token_type == BLOCK)
        {
            if(*token == '{') block = 1; /* ���Ϳ�, û������ */
            else return; /* ��'}', ��ô���� */
        }
        else /* �ǹؼ��� */
        {
            switch(tok)
            {
            case INT:     /* �����ֲ����� */
                putback();
                decl_local();
                break;
            case RETURN:  /* ���򷵻� */
                func_ret();
                return;
            case IF:      /* IF�ؼ��� */
                exec_if();
                break;
            case ELSE:    /* ELSE�ؼ��� */
                find_eob(); 
                break;
            case WHILE:   /* WHILEѭ�� */
                exec_while();
                break;
            case DO:      /* DOѭ�� */
                exec_do();
                break;
            case FOR:     /* FORѭ�� */
                exec_for();
                break;
            }
        }
    } while(tok != FINISHED && block);

}


/* ���������ھֲ�������ջ֮ǰѹջ */
local void get_args(void)
{
    int value=0, count= 0, temp[NUM_PARAMS]={0,};
    var_type i={0};

    get_token();
    if(*token != '(') 
        sntx_err(PAREN_EXPECTED);

    do{
        if(count > NUM_PARAMS)
            sntx_err(TOO_MANY_VARS);
        if(g_error) return;
        eval_exp(&value);
        temp[count] = value;  /* ��ʱ�������ֵ */
        get_token();
        count++;
    }while(*token == ',');
    count--;
    /* ����, ���ر�������ѹջ */
    if(g_error) return;
    for(; count>=0; count--)
    {
        i.value = temp[count];
        local_push(i);
    }  
}
/* �����ֲ�������ջ */
local int func_pop(void)
{
    if(g_error) return 0;
    functos--;
    if(functos < 0) 
        sntx_err(RET_NOCALL);
    return call_stack[functos];
}

/* �����ֲ�����ѹջ */
local void func_push(int i)
{
    if(g_error) return;
    if(functos>NUM_FUNC)
        sntx_err(NEST_FUNC);
    call_stack[functos] = i;
    functos++;

}
/* ���ú���. */
local void call(void)
{
    char *loc, *temp;
    int lvartemp;

    if(g_error) return;
    loc = find_func(token); /* ���Һ������ */
    if(!loc)
        sntx_err(FUNC_UNDEF); /* ����δ���� */
    else
    {
        lvartemp = lvartos;      /* ����ֲ�������ջλ�� */
        get_args();              /* ��ȡ����ֵ  */
        temp = prog;             /* ���淵��λ�� */
        func_push(lvartemp);     /* ����ֲ�����λ�� */
        prog = loc;              /* �ָ�prog��������ʼλ�� */
        get_params();            /* ���غ����Ĳ���ֵ */
        interp_block();          /* ���ͺ��� */
        prog = temp;             /* �ָ�λ�� */
        lvartos = func_pop();    /* ˢ�¾ֲ�������ջ */
    } 
}

/* ��ȡ��������. */
local void get_params(void)
{
    struct var_type *p;
    int i;

    i = lvartos-1;
    do
    { /* �����Ų��� */
        get_token();      
        if(g_error) return;
        p = &local_var_stack[i];
        if(*token != ')' )
        {
            if(tok != INT)
                sntx_err(TYPE_EXPECTED);
            get_token();
            /* ���Ӳ������ƺ;ֲ� ���� ��ջ */
            strcpy(p->var_name, token);
            get_token();
            i--;
        }
        else break;
    }while(*token == ',');
    if(*token != ')') 
        sntx_err(PAREN_EXPECTED);  
}

/* �Ӻ�������. */
local void func_ret(void)
{
    int value = 0;

    /* ��ȡ����ֵ */
    eval_exp(&value);
    ret_value = value;  
}

/* �ֲ�����ѹջ. */
local void local_push(var_type i)
{
    if(g_error) return;
    if(lvartos > NUM_LOCAL_VARS)
        sntx_err(TOO_MANY_VARS);

    local_var_stack[lvartos] = i;
    lvartos++;  
}


//ȷ����ʶ���Ƿ�Ϊ����. ����1��; ���򷵻�0 
local int is_var(char *s)
{
    int i;

    if(g_error) return 0;
    // ���ȿ��Ƿ��Ǿֲ����� 
    for(i=lvartos-1; i >= call_stack[functos-1]; i--)
        if(!strcmp(local_var_stack[i].var_name, s))
            return 1;

    // ������ȫ�ֱ��� 
    for(i=0; i <= gvar_index; i++)
        if(!strcmp(global_vars[i].var_name, s))
            return 1;

    return 0;
}
local int* var_point(char *s)
{
    int i;
    /* ���ȿ��Ƿ��Ǿֲ����� */
    for(i=lvartos-1; i >= call_stack[functos-1]; i--)
    {
        if(!strcmp(local_var_stack[i].var_name, s))
            return &local_var_stack[i].value;
    }
    /* ������Ǿֲ�����������ȫ�ֱ����� */
    for(i=0; i <= gvar_index; i++)
    {
        if(!strcmp(global_vars[i].var_name, s))
            return &global_vars[i].value;
    }
    sntx_err(NOT_VAR); /* ����û���ҵ� */
    return NULL;
}
//����ϵͳ������ֵ
local int find_const(char *s)
{
    int i;

    if(g_error) return -1;
    //���Ȳ鿴�Ƿ��ǹ̶�����
    for(i=0; *constant[i].name; i++)
    {
        if(!strcmp(constant[i].name, s))
            return constant[i].value;
    }
    //�ٲ鿴�Ƿ��Ƕ�̬����
    if(!strcmp("SCRW",s))
        return SCREEN_WIDTH;

    if(!strcmp("SCRH",s))
        return SCREEN_HEIGHT;

    return -1;
}

/* ִ��if */
local void exec_if(void)
{
    int cond;

    if(g_error) return;
    eval_exp(&cond); /* ��ȡ if ���ʽ */
    if(cond)//Ϊ��
    {
        interp_block();
    }
    else
    { /* ���� ����IF block and process the ELSE, if present */
        find_eob(); /* ���ҿ�ʼ����һ�� */
        get_token();

        if(tok != ELSE)
        {
            putback();  /* ���û��ELSE���򷵻� */
            return;
        }
        interp_block();
    }  
}

/* ִ��while. */
local void exec_while(void)
{
    int cond;
    char *temp;

    if(g_error) return;
    putback();
    temp = prog;  /* ����ͷλ�� */
    get_token();
    eval_exp(&cond);  /* �������������ʽ */
    if(cond) 
    {
        interp_block();  /* �����, ���� */
    }
    else
    {  /* ����, ����ѭ�� */
        find_eob();
        return;
    }
    prog = temp;  /* ����ͷλ�� */
}

/* ִ��do. */
local void exec_do(void)
{
    int cond;
    char *temp;

    if(g_error) return;
    putback();
    temp = prog;  /* ����ѭ��λ�� */

    get_token(); /* ��ȡ��ʼѭ�� */
    interp_block(); /* ����ѭ�� */
    get_token();
    if(tok != WHILE) 
        sntx_err(WHILE_EXPECTED);
    eval_exp(&cond); /* ���ѭ������ */
    if(cond) prog = temp; /* ����� ѭ��; ����, ���� */
}

/* ���ҽ�����. */
local void find_eob(void)
{
    int brace;

    get_token();
    brace = 1;
    do
    {
        if(g_error) return;
        get_token();
        if(*token == '{') brace++;
        else if(*token == '}') brace--;
    }while(brace);

}

/* ִ��for. */
local void exec_for(void)
{
    int cond;
    char *temp, *temp2;
    int brace ;

    if(g_error) return;
    get_token();
    eval_exp(&cond);  /* ��ʼ�����ʽ */
    if(*token != ';') sntx_err(SEMI_EXPECTED);
    prog++; /* ���� ; */
    temp = prog;
    while(1)
    {
        if(g_error) return;
        eval_exp(&cond);  /* ������� */
        if(*token != ';') sntx_err(SEMI_EXPECTED);
        prog++; /* ���� ; */
        temp2 = prog;

        /* ����for��ʼ�� */
        brace = 1;
        while(brace)
        {
            if(g_error) return;
            get_token();
            if(*token == '(') brace++;
            if(*token == ')') brace--;
        }

        if(cond) 
        {
            interp_block();  /* �����, ���� */
        }
        else
        {  /* ����, ����ѭ�� */
            find_eob();
            return;
        }
        prog = temp2;
        eval_exp(&cond); /* ������ */
        prog = temp;  /* ѭ���˻ؿ�ʼλ�� */
    }
}

/* ���������. */
local void eval_exp(int *value)
{
    if(g_error) return;
    get_token();
    if(!*token) sntx_err(SYNTAX);
    if(*token == ';')
    {
        *value = 0; /* �ձ��ʽ */
        prog--;
        return;
    }
    eval_exp0(value);
    putback(); /* ��������ȡ�ķ��ŵ������� */
}

/* ����ֵ���ʽ */
local void eval_exp0(int *value)
{
    if(g_error) return;
    if(token_type == IDENTIFIER)
    {
        if(is_var(token))
        {  /* ����Ǳ���, ���Ƿ��Ǹ�ֵ��� */
            char temp[ID_LEN];  /* ������ո�ֵ�ı������� */
            int temp_tok;

            strcpy(temp, token);
            temp_tok = token_type;
            get_token();
            if(*token == '=')
            {  
                int i;

                get_token();
                eval_exp0(value);  /* ��ȡ��ֵ */
                /* ������ֵ. */
                if(g_error) return;
                /* ���ȿ��Ƿ��Ǿֲ����� */
                for(i=lvartos-1; i >= call_stack[functos-1]; i--)
                {
                    if(!strcmp(local_var_stack[i].var_name, temp))
                    {
                        local_var_stack[i].value = *value;
                        return;
                    }
                }
                /* ������Ǿֲ�����������ȫ�ֱ����� */
                for(i=0; i <= gvar_index; i++)
                {
                    if(!strcmp(global_vars[i].var_name, temp))
                    {
                        global_vars[i].value = *value;
                        return;
                    }
                }
                sntx_err(NOT_VAR); /* ����û���ҵ� */
                return;
            }
            else
            {  /* û�и�ֵ */
                putback();  /* �ָ�ԭʼtoken */
                strcpy(token, temp);
                token_type = temp_tok;
            }
        }
    }
    eval_exp1(value);
}

/* ��������㷨 */
local void eval_exp1(int *value)
{
    if(g_error) return;
    eval_exp2(value);
    if(strchr(g_relops, *token))
    {
        int partial_value=0;
        char op;

        op = *token;
        get_token();
        eval_exp2(&partial_value);
        switch(op)
        {
        case LT:
            *value = *value < partial_value;
            break;
        case LE:
            *value = *value <= partial_value;
            break;
        case GT:
            *value = *value > partial_value;
            break;
        case GE:
            *value = *value >= partial_value;
            break;
        case EQ:
            *value = *value == partial_value;
            break;
        case NE:
            *value = *value != partial_value;
            break;
        }
    }
}

/* �ӻ��������. */
local void eval_exp2(int *value)
{
    if(g_error) return;
    eval_exp3(value);
    while(*token == '+' || *token == '-')
    {
        char op;
        int partial_value=0;

        op = *token;
        if(g_error) return;
        get_token();
        eval_exp3(&partial_value);
        switch(op)
        {
        case '-':
            *value = *value - partial_value;
            break;
        case '+':
            *value = *value + partial_value;
            break;
        }
    }
}

/* �˳���ȡ����. */
local void eval_exp3(int *value)
{
    eval_exp4(value);
    while(*token == '*' || *token == '/' || *token == '%')
    {
        char op;
        int partial_value=0;

        op = *token;
        if(g_error) return;
        get_token();
        eval_exp4(&partial_value);
        switch(op)
        {
        case '*':
            *value = *value * partial_value;
            break;
        case '/':
            if(!partial_value) 
            {
                sntx_err(DIV_BY_ZERO); 
                return;
            }
            *value = (*value) / partial_value;
            break;
        case '%':
            *value = (*value) % partial_value;
            break;
        }
    }
}

/* һԪ + �� -. */
local void eval_exp4(int *value)
{
    char op=0;

    if(g_error) return;
    if(*token == '+' || *token == '-')
    {
        op = *token;
        get_token();
    }
    eval_exp5(value);
    if(op == '-') *value = -(*value);
}

/* �������ű��ʽ */
local void eval_exp5(int *value)
{
    if(g_error) return;
    if(*token == '(')
    {
        get_token();
        eval_exp0(value);   /* ��ȡ���ʽ */
        if(*token != ')') 
            sntx_err(PAREN_EXPECTED);
        get_token();
    }
    else
        atom(value);

}

/* ������ֵ, ����, �� ����. */
local void atom(int *value)
{
    if(g_error) return;
    switch(token_type)
    {
    case IDENTIFIER:
        {
            int i;

            i = internal_func(token);
            if(i != -1)/* ����ϵͳ���� */
                *value = (*intern_func[i].p)();
            else
            {
                if(find_func(token)) /* �����Զ��庯�� */
                {
                    call();
                    *value = ret_value;
                }
                else if(-1 != (*value = find_const(token)) );//��ȡϵͳ����ֵ
                else            
                { /* ����ָ��������ֵ */
                    int i;

                    if(g_error) return;
                    /* ���ȿ��Ƿ��Ǿֲ����� */
                    for(i=lvartos-1; i >= call_stack[functos-1]; i--)
                        if(!strcmp(local_var_stack[i].var_name, token))
                        {
                            *value = local_var_stack[i].value;
                            goto aa;
                        }
                    /* ����ȫ�ֱ��� */
                    for(i=0; i <= gvar_index; i++)
                        if(!strcmp(global_vars[i].var_name, token))
                        {
                            *value = global_vars[i].value;
                            goto aa;
                        }
                    //�������
                    sntx_err(NOT_VAR);
                    return;
                }
            }
aa:
            get_token();
        }
        return;
    case NUMBER: /* �����ֳ��� */
        *value = atoi(token);
        get_token();
        return ;
    case DELIMITER:/* �����һ���ַ������Ļ� */
        if(*token == '\'')
        {
            *value = *prog;/* �����'�ַ�����ô�ѵ�ǰ��ֵ�ŵ�value�� */
            prog++;
            if(*prog!='\'')/* ���������'���Ž�β�����׳��﷨���� */
                sntx_err(SYNTAX);
            prog++;
            get_token();
            return ;
        }
        if(*token==')') return ; /* ����ձ��ʽ */
        else sntx_err(SYNTAX); /* �﷨���� */
    default:
        sntx_err(SYNTAX); /* �﷨���� */
    }

}

/* ��ʾ������Ϣ */
local void sntx_err(int error)
{
    char *p, *temp;
    int linecount = 0;
    int i;

    mrc_sprintf(tmp,"\n%s", e[error]);
    if(resultTxtlen<RESULT_SIZE)
    {
        mrc_strcat(ResultString,tmp);
        resultTxtlen+=mrc_strlen(tmp);
    }
    p = p_buf;
    while(p != prog)
    {  /* ���ҳ����� */
        p++;
        if(*p == '\r')
        {
            linecount++;
        }
    }
    mrc_sprintf(tmp,",��%d��\n", linecount); 
    if(resultTxtlen<RESULT_SIZE)
    {
        mrc_strcat(ResultString,tmp);
        resultTxtlen+=mrc_strlen(tmp);
    }

    temp = p;
    for(i=0;     i < 20 && p > p_buf && *p != '\n';      i++, p--);
    {
        for(i=0;     i < 30 && p <= temp;     i++, p++)
        {
            mrc_sprintf(tmp,"%c", *p); 
            if(resultTxtlen<RESULT_SIZE)
            {
                mrc_strcat(ResultString,tmp);
                resultTxtlen+=mrc_strlen(tmp);
            }
        }
    }
    mrc_timerStop(timer[0]);
    mrc_timerStop(timer[1]);
    g_error=TRUE;
}

local int get_token(void)
{

    char *temp;

    if(g_error) return -1;
    token_type = 0; tok = 0;
    temp = token;
    *temp = '\0';
start:
    /* �����հ� */
    while(iswhite(*prog) && *prog) 
        ++prog;

    if(*prog == '\r')
    {
        prog+=2;
        goto start;
    }
    if(*prog == '\0')/* ���� */
    { 
        *token = '\0';
        tok = FINISHED;
        return (token_type = DELIMITER);
    }
    if(*prog & 0x80) goto exit;/* ���ַǷ��ַ� */
    if(strchr("{}", *prog))
    {
        *temp = *prog;
        temp++;
        *temp = '\0';
        prog++;
        return (token_type = BLOCK);
    }
    if(*prog == '/')/* ����ע�� */
    {
        if(prog[1] == '*')/* ����ע�� */
        {
            prog += 2;
            do {
                while(*prog != '*' && *prog)
                    prog++;
                prog++;
            } while(*prog != '/' && *prog);
            prog++;
            goto start;
        }
        else if(prog[1] == '/')
        {
            while(*prog != '\r' && *prog)
                prog++;
            prog+=2;
            goto start;
        }
    }
    if(strchr("!<>=", *prog))
    {
        switch(*prog) 
        {
        case '=': 
            if(prog[1] == '=') 
            {
                prog++; prog++;
                *temp = EQ;
                temp++; *temp = EQ; temp++;
                *temp = '\0';
            }
            break;
        case '!':
            if(prog[1] == '=')
            {
                prog++; prog++;
                *temp = NE;
                temp++; *temp = NE; temp++;
                *temp = '\0';
            }
            break;
        case '<':
            if(prog[1] == '=')
            {
                prog++; prog++;
                *temp = LE; temp++; *temp = LE;
            }
            else
            {
                prog++;
                *temp = LT;
            }
            temp++;
            *temp = '\0';
            break;
        case '>':
            if(prog[1] == '=')
            {
                prog++; prog++;
                *temp = GE; temp++; *temp = GE;
            }
            else
            {
                prog++;
                *temp = GT;
            }
            temp++;
            *temp = '\0';
            break;
        }
        if(*token)
            return (token_type = DELIMITER);
    }
    if(strchr("+-*%/=;()',", *prog)) /* ����ķ��� */
    { 
        *temp = *prog;
        prog++;
        temp++;
        *temp = '\0';
        return (token_type = DELIMITER);
    }
    if(*prog=='"') 
    {
        prog++;
        while(*prog != '"' && *prog != '\r' && *prog)
            *temp++ = *prog++;
        if(*prog == '\r') sntx_err(SYNTAX);
        prog++; *temp = '\0';
        return (token_type = STRING);
    }
    if(isdigit(*prog))
    {
        while(isdigit(*prog) && *prog)//��������������Ч��������С��
            *temp++ = *prog++;
        *temp = '\0';
        return (token_type = NUMBER);
    }
    if(isalpha(*prog))  /* ��ʶ�������� */
    {
        while(isalpha(*prog) || isdigit(*prog) && *prog)
            *temp++ = *prog++;
        *temp = '\0';
        /* ���ǹؼ��ֻ��Ǳ��� */
        tok = look_up(token);
        if(tok)
            token_type = KEYWORD; /* �ǹؼ��� */
        else
            token_type = IDENTIFIER;
        return token_type;
    }
exit:
    sntx_err(SYNTAX);
    return -1;
}


local int look_up(char *s)
{
    int i;

    if(g_error) return 0;

    for(i=0; *table[i].command; i++)
    {
        if(!strcmp(table[i].command, s))
            return table[i].tok;
    }
    return 0; /* δ֪���� */
}

/* �����ڲ�����λ�ã�δ�ҵ����� -1 */
local int internal_func(char *s)
{
    int i;

    if(g_error) return -1;
    for(i=0; intern_func[i].f_name[0]; i++) 
    {
        if(!strcmp(intern_func[i].f_name, s)) 
            return i;
    }
    return -1;
}

/* �������к����ͱ���ȫ�ֱ���. */
local void prescan(void)
{
    char *p, *tp;
    char temp[80];
    int brace = 0;  /* Ϊ0ʱ��ǰԴλ�����ⲿ���� */

    p = prog;//����ͷָ��
    do {
        while(brace)
        {
            get_token();
            if(g_error) return;
            if(tok == FINISHED)
                sntx_err(SYNTAX);
            if(*token == '{') brace++;
            if(*token == '}') brace--;
        }
        tp = prog; /* ���浱ǰλ�� */
        get_token();
        if(g_error) return;//����Ҫ���,�������������
        if(tok==INT)/* ȫ�ֱ������ͻ����������� */
        {
            get_token();
            if(token_type == IDENTIFIER)
            {
                strcpy(temp, token);
                get_token();
                /* ��ȫ�ֱ��� ,����ȫ�ֱ���*/
                if(*token != '(')
                {
                    prog = tp; /* ���ص���ʼ���� */
                    get_token();  /* ��ȡ���� */
                    do {
                        if(g_error) return;
                        if(gvar_index > NUM_GLOBAL_VARS) 
                        {
                            sntx_err(TOO_MANY_VARS);
                            return;
                        }
                        global_vars[gvar_index].value = 0;  /* ��ʼ��Ϊ0 */
                        get_token();  /* ��ȡ���� */
                        strncpy(global_vars[gvar_index].var_name, token,ID_LEN-1);
                        gvar_index++;
                        get_token();
                    } while(*token == ',');
                    if(*token != ';')
                        sntx_err(SEMI_EXPECTED);

                }
                /* �Ǻ��� ,��������*/
                else if(*token == '(')
                {
                    if(func_index > NUM_FUNC) 
                    {
                        sntx_err(TOO_MANY_FUNC);
                        return;
                    }
                    func_table[func_index].loc = prog;//--prog
                    strncpy(func_table[func_index].func_name, temp,ID_LEN-1);
                    func_index++;
                    while(*prog != ')' && *prog)
                        prog++;

                    prog++;
                }
                else putback();
            }
        }
        else if(*token == '{') 
            brace++;

    } while(tok != FINISHED);
    prog = p;//�ָ�ͷָ��

}

/* ����ָ����ں���. ���û�ҵ����� NULL */
local char *find_func(char *name)
{
    int i;

    if(g_error) return NULL;
    for(i=0; i <= func_index; i++)
        if(!strcmp(name, func_table[i].func_name))
            return func_table[i].loc;
    return NULL;
}

/* �����ֲ�����. */
local void decl_local(void)
{
    struct var_type i;

    get_token();  /* ��ȡ���� */
    i.value = 0;  /* ��ʼ��Ϊ0 */
    do
    { /* ������ */
        if(g_error) return;
        get_token(); /* ��ȡ�������� */
        strcpy(i.var_name, token);
        local_push(i);
        get_token();
    } while(*token == ',');
    if(*token != ';')
        sntx_err(SEMI_EXPECTED);
}

char* GetResult(void)
{
    return ResultString;
}
local void InitVariable(void)
{
    token_type=tok=g_error = lc_state = 0;
    resultTxtlen=functos=func_index=gvar_index=lvartos=ret_value=0;
    mrc_memset(call_stack,0,sizeof(call_stack));
    mrc_memset(local_var_stack,0,sizeof(local_var_stack));
    mrc_memset(global_vars,0,sizeof(global_vars));
    mrc_memset(func_table,0,sizeof(func_table));
    mrc_memset(ResultString,0,RESULT_SIZE);
    mrc_memset(tmp,0,sizeof(tmp));
}

int32 InitLittleC(void)
{
    ResultString = NULL;
    ResultString = (char*)mrc_malloc(RESULT_SIZE);
    if(!ResultString) return -1;
    timer[0] = mrc_timerCreate();
    if(!timer[0]) return -1;
    timer[1] = mrc_timerCreate();
    if(!timer[1]) return -1;

    //////////////
    intern_func[FUNC_POINT].f_name = "point";
    intern_func[FUNC_REFRESH].f_name = "refresh";
    intern_func[FUNC_LINE].f_name = "line";
    intern_func[FUNC_RECT].f_name = "rect";
    intern_func[FUNC_DRAWTXT].f_name = "drawtxt";
    intern_func[FUNC_DRAWTXT2].f_name = "drawtxt2";
    intern_func[FUNC_DRAWBMP].f_name = "bmp565";
    intern_func[FUNC_CLS].f_name = "cls";
    intern_func[FUNC_SLEEP].f_name = "sleep";
    intern_func[FUNC_SRECT].f_name = "srect";
    intern_func[FUNC_EFFSETCON].f_name = "effsetcon";
    intern_func[FUNC_IMG].f_name = "img";
    intern_func[FUNC_RAND].f_name = "rand";
    intern_func[FUNC_STRW].f_name = "strw";
    intern_func[FUNC_STRH].f_name = "strh";
    intern_func[FUNC_TIMER].f_name = "timer";
    intern_func[FUNC_TIMERSTOP].f_name = "timerstop";
    intern_func[FUNC_IPRINT].f_name = "iprint";
    intern_func[FUNC_PUTCH].f_name = "putch";
    intern_func[FUNC_PRINT].f_name = "print";
    intern_func[FUNC_STOPSOUND].f_name = "stopsound";
    intern_func[FUNC_PLAYSOUND].f_name = "playsound";
    intern_func[FUNC_SMS].f_name = "sms";
    intern_func[FUNC_PRINTSCREEN].f_name = "printscr";
    intern_func[FUNC_RUNMRP].f_name = "runmrp";
    intern_func[FUNC_MKDIR].f_name = "mkdir";
    intern_func[FUNC_REMOVE].f_name = "remove";
    intern_func[FUNC_OPEN].f_name = "open";
    intern_func[FUNC_CLOSE].f_name = "close";
    intern_func[FUNC_READ].f_name = "read";
    intern_func[FUNC_SEEK].f_name = "seek";
    intern_func[FUNC_WRITE].f_name = "write";
    intern_func[FUNC_GETLEN].f_name = "getlen";
    intern_func[FUNC_RENAME].f_name = "rename";
    intern_func[FUNC_EXIT].f_name = "exit";
    intern_func[FUNC_END].f_name = "";

    intern_func[FUNC_POINT].p = call_point;
    intern_func[FUNC_REFRESH].p = call_refresh;
    intern_func[FUNC_LINE].p = call_line;
    intern_func[FUNC_RECT].p = call_rect;
    intern_func[FUNC_DRAWTXT].p = call_drawtxt;
    intern_func[FUNC_DRAWTXT2].p = call_drawtxt2;
    intern_func[FUNC_DRAWBMP].p = call_draw565bmp;
    intern_func[FUNC_CLS].p = call_cls;
    intern_func[FUNC_SLEEP].p = call_sleep;
    intern_func[FUNC_SRECT].p = call_srect;
    intern_func[FUNC_EFFSETCON].p = call_effsetcon;
    intern_func[FUNC_IMG].p = call_img;
    intern_func[FUNC_RAND].p = call_rand;
    intern_func[FUNC_STRW].p = call_strw;
    intern_func[FUNC_STRH].p = call_strh;
    intern_func[FUNC_TIMER].p = call_timerStart;
    intern_func[FUNC_TIMERSTOP].p = call_timerStop;
    intern_func[FUNC_IPRINT].p = call_iprint;
    intern_func[FUNC_PUTCH].p = call_putch;
    intern_func[FUNC_PRINT].p = call_print;
    intern_func[FUNC_STOPSOUND].p = call_stopsound;
    intern_func[FUNC_PLAYSOUND].p = call_playsound;
    intern_func[FUNC_SMS].p = call_sms;
    intern_func[FUNC_PRINTSCREEN].p = call_printscr;
    intern_func[FUNC_RUNMRP].p = call_runmrp;
    intern_func[FUNC_MKDIR].p = call_mkdir;
    intern_func[FUNC_REMOVE].p = call_remove;
    intern_func[FUNC_OPEN].p = call_open;
    intern_func[FUNC_CLOSE].p = call_close;
    intern_func[FUNC_READ].p = call_read;
    intern_func[FUNC_SEEK].p = call_seek;
    intern_func[FUNC_WRITE].p = call_write;
    intern_func[FUNC_GETLEN].p = call_getlen;
    intern_func[FUNC_RENAME].p = call_rename;

    intern_func[FUNC_EXIT].p = call_exit;
    intern_func[FUNC_END].p = 0;
    ////////////
    e[SYNTAX] = "�﷨����";
    e[NOT_VAR] = "�Ǳ���������";
    e[TYPE_EXPECTED] = "��������";
    e[SEMI_EXPECTED] = "ȱ��';'";
    e[FUNC_UNDEF] = "����δ����";
    e[RET_NOCALL] = "û��return";
    e[PAREN_EXPECTED] = "ȱ������";
    e[WHILE_EXPECTED] = "ȱ��while";
    e[NEST_FUNC] = "Ƕ�׹���";
    e[TOO_MANY_VARS] = "�������������";
    e[TOO_MANY_FUNC] = "��������";
    e[DIV_BY_ZERO] = "��������";

    return 0;
}

void ReleaseLittleC(void)
{
    if(timer[0])
        mrc_timerDelete(timer[0]);
    if(timer[1])
        mrc_timerDelete(timer[1]);

    if(ResultString) mrc_free(ResultString);
}

int StartLittleC(char *mem)
{    
    InitVariable();
    prog = p_buf = mem;        /* ���ó�����ʼָ�� */
    prescan();                 /* �ڳ����в������к���λ�ú�ȫ�ֱ����������ͳ�ʼ�� */
    if(g_error) goto aa;
    prog = find_func("main");  //���Һ������
    if(!prog)
    {
        if(resultTxtlen<RESULT_SIZE)
        {
            mrc_strcat(ResultString,"û��int main()\n");
            resultTxtlen+=mrc_strlen("û��int main()\n");
        }
        goto aa;
    }
    strcpy(token, "main");
    prog--;
    call(); /* ��main��ʼ���� */
    if(g_error) goto aa;
    return 0;
aa:
    return -1;
}
int StartLittleCFunc(char *func)
{
    token_type=tok=g_error=0;
    prog = find_func(func);  //���Һ������
    if(!prog)
        return 1;
    strcpy(token, func);
    prog--;
    call(); /* ��func��ʼ���� */
    if(g_error) return -1;

    return 0;
}

int StartLittleCEvent(int code, int p1, int p2)
{
    char *loc, *temp;
    int lvartemp;

    token_type=tok=g_error=0;
    loc = find_func("event"); /* ���Һ������ */
    if(!loc)
        return 1;
    else
    {
        var_type i={0};

        lvartemp = lvartos;      /* ����ֲ�������ջλ�� */

        //P2ֵѹջ
        i.value = p2;
        local_push(i);
        //P1ֵѹջ
        i.value = p1;
        local_push(i);
        //codeֵѹջ
        i.value = code;
        local_push(i);

        temp = prog;             /* ���淵��λ�� */
        func_push(lvartemp);     /* ����ֲ�����λ�� */
        prog = loc;              /* �ָ�prog��������ʼλ�� */
        get_params();            /* ���غ����Ĳ���ֵ */
        interp_block();          /* ���ͺ��� */
        prog = temp;             /* �ָ�λ�� */
        lvartos = func_pop();    /* ˢ�¾ֲ�������ջ */
    } 
    if(g_error) return -1;
    return 0;
}

int littleCStop(void)
{
    mrc_timerStop(timer[0]);
    mrc_timerStop(timer[1]);
    return 0;
}