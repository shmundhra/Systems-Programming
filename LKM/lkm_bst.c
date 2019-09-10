#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define PB2_SET_TYPE _IOW(0x10, 0x31, int32_t*)
#define PB2_SET_ORDER _IOW(0x10, 0x32, int32_t*)
#define PB2_GET_INFO _IOR(0x10, 0x33, int32_t*)
#define PB2_GET_OBJ _IOR(0x10, 0x34, int32_t*)

#define INORDER 0x00
#define PREORDER 0x01
#define POSTORDER 0x02

#define NONE 0x00
#define NUM  0xFF
#define STR  0xF0

#define MAX 101
#define INF 100000

typedef struct obj_info_{
    __INT32_TYPE__ deg1cnt;     /* Number of nodes with degree 1 (in or out) */
    __INT32_TYPE__ deg2cnt;     /* Number of nodes with degree 2 (in or out) */
    __INT32_TYPE__ deg3cnt;     /* Number of nodes with degree 3 (in or out) */
    __INT32_TYPE__ maxdepth;    /* Maximum Number of Edges from Root to a Leaf */
    __INT32_TYPE__ mindepth;    /* Minimum Number of Edges from Root to a Leaf */
}obj_info;

typedef struct search_obj_{
    unsigned char objtype;      /* Either 0xFF or 0xF0 represent Integer or String */
    char found;                 /* if (Found == 1) then Found; else Not Found */
    __INT32_TYPE__ int_obj;     /* Value of integer. Valid only if objtype == NUM */
    char str[MAX];              /* Value of string.  Valid only if objtype == STR */
    __INT32_TYPE__ len;         /* Length of string. Valid only if objtype == STR */
}search_obj;

typedef struct i_node_{
    __INT32_TYPE__ value;
    __INT32_TYPE__ height;
    __INT32_TYPE__ degree;
    struct i_node_* left;
    struct i_node_* right;
}i_node;

typedef struct i_stack_node_{
    i_node* element;
    struct i_stack_node_* next;
} i_stack_node;


typedef struct s_node_{
    char value[MAX];
    __INT32_TYPE__ height;
    __INT32_TYPE__ degree;
    struct s_node_* left;
    struct s_node_* right;
}s_node;

typedef struct s_stack_node_{
    s_node* element;
    struct s_stack_node_* next;
} s_stack_node;


typedef struct pcb_{
    pid_t proc_pid;             /* Stores the processID of the owner process. */
    __INT16_TYPE__ objType;     /* Stores 0xFF(NUM) or 0xF0(STR) */
    __INT16_TYPE__ orderType;   /* Stores 0x00, 0x01, 0x02 for IN, PRE, POST ORDER */
    i_node* i_root;             /* Root of NUM BST,  Valid only if objtype == NUM */
    s_node* s_root;             /* Root of STR BST,  Valid only if objtype == STR */
    i_stack_node* i_top;        /* Computation Stack, Valid only if objtype == NUM */
    s_stack_node* s_top;        /* Computation Stack, Valid only if objtype == STR */
    struct pcb_* next;          /* Stores link to next Block in the List */
}pcb;

static int open(struct inode *inodep, struct file *filep);
static long ioctl(struct file *file, unsigned int cmd, unsigned long arg);
static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos);
static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos);
static int release(struct inode *inodep, struct file *filep);

static pcb* pcb_node_Create(pid_t pid);
static void pcb_node_Reset(pcb* node);
static pcb* pcb_list_Insert(pid_t pid);
static pcb* pcb_list_Get(pid_t pid);
static int pcb_list_Delete(pid_t pid);

obj_info* obj_info_Create(__INT32_TYPE__ deg1,
                          __INT32_TYPE__ deg2,
                          __INT32_TYPE__ deg3,
                          __INT32_TYPE__ maxd,
                          __INT32_TYPE__ mind);

static i_node* i_node_Create(__INT32_TYPE__ val,
                             __INT32_TYPE__ ht,
                             __INT32_TYPE__ deg,
                             i_node* l,
                             i_node* r);
static i_stack_node* i_stack_Create(i_node* elem);
static __INT8_TYPE__ i_stack_Empty(i_stack_node** i_top);
static i_stack_node* i_stack_Push(i_stack_node** i_top, i_node* elem);
static i_node* i_stack_Pop(i_stack_node** i_top);
static i_node* i_stack_Top(i_stack_node** i_top);

static void i_reset_cursor(i_stack_node** i_top, i_node* i_root);
static i_node* i_bst_Search(i_node* i_root, __INT32_TYPE__ val);
static i_node* i_bst_Insert(i_node** i_root, __INT32_TYPE__ val);
static void i_bst_Delete(i_node** i_root);
static i_node* i_bst_Preorder_Next(i_stack_node** i_top);
static i_node* i_bst_Inorder_Next(i_stack_node** i_top);
static i_node* i_bst_Postorder_Next(i_stack_node** i_top);
static ssize_t i_bst_Info(i_node* i_root, obj_info* info);


static s_node* s_node_Create(char* val,
                             __INT32_TYPE__ ht,
                             __INT32_TYPE__ deg,
                             s_node* l,
                             s_node* r);
static s_stack_node* s_stack_Create(s_node* elem);
static __INT8_TYPE__ s_stack_Empty(s_stack_node** s_top);
static s_stack_node* s_stack_Push(s_stack_node** s_top, s_node* elem);
static s_node* s_stack_Pop(s_stack_node** s_top);
static s_node* s_stack_Top(s_stack_node** s_top);

static void s_reset_cursor(s_stack_node** s_top, s_node* s_root);
static s_node* s_bst_Search(s_node* s_root, char* val);
static s_node* s_bst_Insert(s_node** s_root, char* val);
static void s_bst_Delete(s_node** s_root);
static s_node* s_bst_Preorder_Next(s_stack_node** s_top);
static s_node* s_bst_Inorder_Next(s_stack_node** s_top);
static s_node* s_bst_Postorder_Next(s_stack_node** s_top);
static ssize_t s_bst_Info(s_node* s_root, obj_info* info);

static struct file_operations file_ops;
static DEFINE_MUTEX(pcb_mutex);
static pcb* pcb_Head = NULL;

MODULE_LICENSE("GPL");

static pcb* pcb_node_Create(pid_t pid) {
    pcb* node = (pcb*)kmalloc(sizeof(pcb), GFP_KERNEL);

    node->proc_pid = pid;
    node->objType = NONE;
    node->orderType = INORDER;
    node->i_root = NULL;
    node->s_root = NULL;
    node->i_top = NULL;
    node->s_top = NULL;
    node->next = NULL;
    return node;
}

static void pcb_node_Reset(pcb* node) {
    node->objType = NONE;
    node->orderType = INORDER;

    i_bst_Delete(&(node->i_root));
    i_reset_cursor(&(node->i_top), node->i_root);

    s_bst_Delete(&(node->s_root));
    s_reset_cursor(&(node->s_top), node->s_root);

    return;
}

static pcb* pcb_list_Insert(pid_t pid) {
    pcb* node = pcb_node_Create(pid);

    node->next = pcb_Head;
    pcb_Head = node;
    return node;
}

static pcb* pcb_list_Get(pid_t pid) {
    pcb* tmp = pcb_Head;

    while(tmp){
        if (tmp->proc_pid == pid) {
            return tmp;
        }
        tmp = tmp->next;
    }
    return NULL;
}

static int pcb_list_Delete(pid_t pid) {
    pcb* prev = NULL;
    pcb* curr = NULL;

    if (pcb_Head->proc_pid == pid) {
        curr = pcb_Head;
        pcb_Head = pcb_Head->next;
        kfree(curr);
        return 0;
    }

    prev = pcb_Head;
    curr = prev->next;
    while(curr) {
        if (curr->proc_pid == pid) {
            prev->next = curr->next;
            kfree(curr);
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;
}

obj_info* obj_info_Create(__INT32_TYPE__ deg1,
                          __INT32_TYPE__ deg2,
                          __INT32_TYPE__ deg3,
                          __INT32_TYPE__ maxd,
                          __INT32_TYPE__ mind) {
    obj_info* node = (obj_info*)kmalloc(sizeof(obj_info), GFP_KERNEL);

    node->deg1cnt = deg1;
    node->deg2cnt = deg2;
    node->deg3cnt = deg3;
    node->maxdepth = maxd;
    node->mindepth = mind;
    return node;
}

static i_node* i_node_Create(__INT32_TYPE__ val,
                      __INT32_TYPE__ ht,
                      __INT32_TYPE__ deg,
                      i_node* l,
                      i_node*r) {
    i_node* node = (i_node*)kmalloc(sizeof(i_node), GFP_KERNEL);

    node->value = val;
    node->height = ht;
    node->degree = deg;
    node->left = l;
    node->right = r;
    return node;
}

static i_stack_node* i_stack_Create(i_node* elem) {
    i_stack_node* node = (i_stack_node*)kmalloc(sizeof(i_stack_node), GFP_KERNEL);

    (node->element) = elem;
    node->next = NULL;
    return node;
}

static __INT8_TYPE__ i_stack_Empty(i_stack_node** i_top) {
    if ((*i_top) == NULL) {
        return 1;
    }
    return 0;
}

static i_stack_node* i_stack_Push(i_stack_node** i_top, i_node* elem) {
    i_stack_node* node = i_stack_Create(elem);

    node->next = (*i_top);
    (*i_top) = node;
    printk(KERN_DEBUG "\t\t\t\t\tPushing %d\n", (node->element)->value);
    return node;
}

static i_node* i_stack_Pop(i_stack_node** i_top) {
    i_stack_node* node = NULL;

    if (i_stack_Empty(i_top)) {
        return NULL;
    }
    node = (*i_top);
    printk(KERN_DEBUG "\t\t\t\t\tPopping %d\n", (node->element)->value);
    (*i_top) = node->next;
    node->next = NULL;

    return ((node->element));
}

static i_node* i_stack_Top(i_stack_node** i_top) {
    if (i_stack_Empty(i_top)) {
        return NULL;
    }
    return ((*i_top)->element);
}

static void i_reset_cursor(i_stack_node** i_top, i_node* i_root) {
    i_node* copy_root = NULL;

    while(*i_top) {
        i_stack_node* temp = NULL;
        temp = (*i_top)->next;
        kfree(*i_top);
        *i_top = temp;
    }
    if (i_root == NULL) {
        return;
    }
    copy_root =  i_node_Create(i_root->value, i_root->height, i_root->degree, i_root->left,  i_root->right);
    i_stack_Push(i_top, copy_root);
    return;
}

static i_node* i_bst_Search(i_node* i_root, __INT32_TYPE__ val) {
    i_stack_node* i_top = NULL;
    i_node* node = NULL;

    if (i_root == NULL) {
        return NULL;
    }
    i_stack_Push(&i_top, i_root);

    while (!i_stack_Empty(&i_top)) {
        node = i_stack_Pop(&i_top);

        if (val == node->value) {
            return node;
        }

        if (val < node->value) {
            if (node->left == NULL) {
                return NULL;
            }
            i_stack_Push(&i_top, node->left);
        } else {
            if (node->right == NULL) {
                return NULL;
            }
            i_stack_Push(&i_top, node->right);
        }
    }
    return NULL;
}

static i_node* i_bst_Insert(i_node** i_root, __INT32_TYPE__ val) {
    i_stack_node* i_top = NULL;
    i_node* node = NULL;

    if (*i_root == NULL) {
        *i_root = i_node_Create(val, 0, 0, NULL, NULL);
        printk(KERN_DEBUG "****Inserting %d at the ROOT****\n", val);
        return NULL;
    }
    i_stack_Push(&i_top, *i_root);

    while (!i_stack_Empty(&i_top)) {
        node = i_stack_Pop(&i_top);

        if (val < node->value) {
            if (node->left == NULL) {
                node->left = i_node_Create(val, node->height + 1, 1, NULL, NULL);
                printk(KERN_DEBUG "****Inserting %d to the LEFT of %d****\n", val, node->value);
                node->degree += 1;
                break;
            }
            i_stack_Push(&i_top, node->left);
        } else {
            if (node->right == NULL) {
                node->right = i_node_Create(val, node->height + 1, 1, NULL, NULL);
                printk(KERN_DEBUG "****Inserting %d to the RIGHT of %d****\n", val, node->value);
                node->degree += 1;
                break;
            }
            i_stack_Push(&i_top, node->right);
        }
    }
    return node;
}

static void i_bst_Delete(i_node** i_root) {
    i_stack_node* i_top = NULL;

    if (*i_root == NULL) {
        return;
    }

    i_stack_Push(&i_top, *i_root);
    while(1) {
        i_node* node = i_stack_Pop(&i_top);
        if (node == NULL) {
            break;
        }

        if (node->right) {
            i_stack_Push(&i_top, node->right);
        }
        if (node->left) {
            i_stack_Push(&i_top, node->left);
        }

        kfree(node);
    }
    (*i_root) = NULL;
}

static i_node* i_bst_Preorder_Next(i_stack_node** i_top) {
    i_node* node = i_stack_Pop(i_top);
    if (node == NULL) {
        return NULL;
    }

    if (node->right) {
        i_node* R = node->right;
        i_node* copy_root = i_node_Create(R->value, R->height, R->degree,
                                          R->left,  R->right);
        i_stack_Push(i_top, copy_root);
    }
    if (node->left) {
        i_node* L = node->left;
        i_node* copy_root = i_node_Create(L->value, L->height, L->degree,
                                          L->left,  L->right);
        i_stack_Push(i_top, copy_root);
    }
    return node;
}

static i_node* i_bst_Inorder_Next(i_stack_node** i_top) {
    i_node* node = i_stack_Top(i_top);

    if (node == NULL) {
        return NULL;
    }

    while (node->left) {
        i_node* L = node->left;
        i_node* copy_root = i_node_Create(L->value, L->height, L->degree,
                                          L->left,  L->right);
        i_stack_Push(i_top, copy_root);

        node->left = NULL;
        node = i_stack_Top(i_top);
    }
    /* Now node holds Top which has no left child */
    /* Shall Pop it and add its right child       */

    node = i_stack_Pop(i_top);
    if (node->right) {
        i_node* R = node->right;
        i_node* copy_root = i_node_Create(R->value, R->height, R->degree,
                                          R->left,  R->right);
        i_stack_Push(i_top, copy_root);
    }

    node->right = NULL;
    return node;
}

static i_node* i_bst_Postorder_Next(i_stack_node** i_top) {
    i_node* node = i_stack_Top(i_top);

    if (node == NULL) {
        return NULL;
    }

    while (1) {
        while (node->left) {
            i_node* L = node->left;
            i_node* copy_root = i_node_Create(L->value, L->height, L->degree,
                                              L->left,  L->right);
            i_stack_Push(i_top, copy_root);

            node->left = NULL;
            node = i_stack_Top(i_top);
        }
        if (node->right) {
            i_node* R = node->right;
            i_node* copy_root = i_node_Create(R->value, R->height, R->degree,
                                              R->left,  R->right);
            i_stack_Push(i_top, copy_root);

            node->right = NULL;
            node = i_stack_Top(i_top);
        } else {
            break;
        }
    }
    node = i_stack_Pop(i_top);
    return node;
}

static ssize_t i_bst_Info(i_node* i_root, obj_info* info) {
    i_stack_node* i_top = NULL;
    ssize_t i_bst_size = 0;

    if (i_root == NULL) {
        return 0;
    }

    i_reset_cursor(&i_top, i_root);
    while (1) {
        i_node* node = i_bst_Preorder_Next(&i_top);
        if (node == NULL) {
            break;
        }

        i_bst_size += sizeof(i_node);

        switch (node->degree) {
            case 1: info->deg1cnt++; break;
            case 2: info->deg2cnt++; break;
            case 3: info->deg3cnt++; break;
            default : break;
        }

        if (!node->left && !node->right) {
            info->mindepth = min((info->mindepth), (node->height));
            info->maxdepth = max((info->maxdepth), (node->height));
        }

        kfree(node);
        printk(KERN_DEBUG "Info goes as: %d %d %d %d %d\n", info->deg1cnt, info->deg2cnt,
                                           info->deg3cnt, info->maxdepth, info->mindepth);
    }
    return i_bst_size;
}


static s_node* s_node_Create(char* val,
                      __INT32_TYPE__ ht,
                      __INT32_TYPE__ deg,
                      s_node* l,
                      s_node* r) {
    s_node* node = (s_node*)kmalloc(sizeof(s_node), GFP_KERNEL);

    memset(node->value, 0, 100);
    strcpy(node->value, val);
    node->height = ht;
    node->degree = deg;
    node->left = l;
    node->right = r;
    return node;
}

static s_stack_node* s_stack_Create(s_node* elem) {
    s_stack_node* node = (s_stack_node*)kmalloc(sizeof(s_stack_node), GFP_KERNEL);

    (node->element) = elem;
    node->next = NULL;
    return node;
}

static __INT8_TYPE__ s_stack_Empty(s_stack_node** s_top) {
    if ((*s_top) == NULL) {
        return 1;
    }
    return 0;
}

static s_stack_node* s_stack_Push(s_stack_node** s_top, s_node* elem) {
    s_stack_node* node = s_stack_Create(elem);

    node->next = (*s_top);
    (*s_top) = node;
    printk(KERN_DEBUG "\t\t\t\t\tPushing %s\n", (node->element)->value);
    return node;
}

static s_node* s_stack_Pop(s_stack_node** s_top) {
    s_stack_node* node = NULL;

    if (s_stack_Empty(s_top)) {
        return NULL;
    }
    node = (*s_top);
    printk(KERN_DEBUG "\t\t\t\t\tPopping %s\n", (node->element)->value);
    (*s_top) = node->next;
    node->next = NULL;
    return node->element;
}

static s_node* s_stack_Top(s_stack_node** s_top) {
    if (s_stack_Empty(s_top)) {
        return NULL;
    }
    return ((*s_top)->element);
}

static void s_reset_cursor(s_stack_node** s_top, s_node* s_root) {
    s_node* copy_root = NULL;

    while(*s_top) {
        s_stack_node* temp = (*s_top)->next;
        kfree(*s_top);
        *s_top = temp;
    }
    if (s_root == NULL) {
        return;
    }
    copy_root = s_node_Create(s_root->value, s_root->height, s_root->degree, s_root->left,  s_root->right);
    s_stack_Push(s_top, copy_root);
    return;
}

static s_node* s_bst_Search(s_node* s_root, char* val) {
    s_stack_node* s_top = NULL;
    s_node* node = NULL;

    if (s_root == NULL) {
        return NULL;
    }
    s_stack_Push(&s_top, s_root);

    while (!s_stack_Empty(&s_top)) {
        node = s_stack_Pop(&s_top);

        if (strcmp(val, node->value) == 0) {
            return node;
        }

        if (strcmp(val, node->value) < 0) {
            if (node->left == NULL) {
                return NULL;
            }
            s_stack_Push(&s_top, node->left);
        } else {
            if (node->right == NULL) {
                return NULL;
            }
            s_stack_Push(&s_top, node->right);
        }
    }
    return NULL;
}

static s_node* s_bst_Insert(s_node** s_root, char* val) {
    s_stack_node* s_top = NULL;
    s_node* node = NULL;

    if (*s_root == NULL) {
        *s_root = s_node_Create(val, 0, 0, NULL, NULL);
        printk(KERN_DEBUG "****Inserting %s at the ROOT****\n", val);
        return NULL;
    }
    s_stack_Push(&s_top, *s_root);

    while (!s_stack_Empty(&s_top)) {
        node = s_stack_Pop(&s_top);

        if (strcmp(val, node->value) < 0) {
            if (node->left == NULL) {
                node->left = s_node_Create(val, node->height + 1, 1, NULL, NULL);
                printk(KERN_DEBUG "****Inserting %s to the LEFT of %s****\n", val, node->value);
                node->degree += 1;
                break;
            }
            s_stack_Push(&s_top, node->left);
        } else {
            if (node->right == NULL) {
                node->right = s_node_Create(val, node->height + 1, 1, NULL, NULL);
                printk(KERN_DEBUG "****Inserting %s to the RIGHT of %s****\n", val, node->value);
                node->degree += 1;
                break;
            }
            s_stack_Push(&s_top, node->right);
        }
    }
    return node;
}

static void s_bst_Delete(s_node** s_root) {
    s_stack_node* s_top = NULL;

    if (*s_root == NULL) {
        return;
    }

    s_stack_Push(&s_top, *s_root);
    while(1) {
        s_node* node = s_stack_Pop(&s_top);
        if (node == NULL) {
            break;
        }

        if (node->right) {
            s_stack_Push(&s_top, node->right);
        }
        if (node->left) {
            s_stack_Push(&s_top, node->left);
        }

        kfree(node);
    }
    (*s_root) = NULL;
}

static s_node* s_bst_Preorder_Next(s_stack_node** s_top) {
    s_node* node = s_stack_Pop(s_top);

    if (node == NULL) {
        return NULL;
    }

    if (node->right) {
        s_node* R = node->right;
        s_node* copy_root = s_node_Create(R->value, R->height, R->degree,
                                          R->left,  R->right);
        s_stack_Push(s_top, copy_root);
    }
    if (node->left) {
        s_node* L = node->left;
        s_node* copy_root = s_node_Create(L->value, L->height, L->degree,
                                          L->left,  L->right);
        s_stack_Push(s_top, copy_root);
    }
    return node;
}

static s_node* s_bst_Inorder_Next(s_stack_node** s_top) {
    s_node* node = s_stack_Top(s_top);

    if (node == NULL) {
        return NULL;
    }

    while (node->left) {
        s_node* L = node->left;
        s_node* copy_root = s_node_Create(L->value, L->height, L->degree,
                                          L->left,  L->right);
        s_stack_Push(s_top, copy_root);

        node->left = NULL;
        node = s_stack_Top(s_top);
    }
    /* Now node holds Top which has no left child */
    /* Shall Pop it and add its right child       */

    node = s_stack_Pop(s_top);
    if (node->right) {
        s_node* R = node->right;
        s_node* copy_root = s_node_Create(R->value, R->height, R->degree,
                                          R->left,  R->right);
        s_stack_Push(s_top, copy_root);
    }

    node->right = NULL;
    return node;
}

static s_node* s_bst_Postorder_Next(s_stack_node** s_top) {
    s_node* node = s_stack_Top(s_top);

    if (node == NULL) {
        return NULL;
    }

    while (1) {
        while (node->left) {
            s_node* L = node->left;
            s_node* copy_root = s_node_Create(L->value, L->height, L->degree,
                                              L->left,  L->right);
            s_stack_Push(s_top, copy_root);

            node->left = NULL;
            node = s_stack_Top(s_top);
        }
        if (node->right) {
            s_node* R = node->right;
            s_node* copy_root = s_node_Create(R->value, R->height, R->degree,
                                              R->left,  R->right);
            s_stack_Push(s_top, copy_root);

            node->right = NULL;
            node = s_stack_Top(s_top);
        } else {
            break;
        }
    }
    node = s_stack_Pop(s_top);
    return node;
}

static ssize_t s_bst_Info(s_node* s_root, obj_info* info) {
    s_stack_node* s_top = NULL;
    ssize_t s_bst_size = 0;

    if (s_root == NULL) {
        return 0;
    }

    s_reset_cursor(&s_top, s_root);
    while (1) {
        s_node* node = s_bst_Preorder_Next(&s_top);
        if (node == NULL) {
            break;
        }

        s_bst_size += sizeof(s_node);

        switch (node->degree) {
            case 1: info->deg1cnt++; break;
            case 2: info->deg2cnt++; break;
            case 3: info->deg3cnt++; break;
            default : break;
        }

        if (!node->left && !node->right) {
            info->mindepth = min((info->mindepth), (node->height));
            info->maxdepth = max((info->maxdepth), (node->height));
        }

        kfree(node);
        printk(KERN_DEBUG "Info goes as: %d %d %d %d %d\n", info->deg1cnt, info->deg2cnt,
                                          info->deg3cnt, info->maxdepth, info->mindepth);
    }
    return s_bst_size;
}

static int open(struct inode *inodep, struct file *filep) {
    pid_t pid = current->pid;

    while(!mutex_trylock(&pcb_mutex));

    if (pcb_list_Get(pid)) {
        mutex_unlock(&pcb_mutex);
        printk(KERN_ERR "open() - File already Open in Process: %d\n", pid);
        return -EACCES;
    }
    pcb_list_Insert(pid);
    mutex_unlock(&pcb_mutex);

    printk(KERN_NOTICE "open() - File Opened by Process: %d\n", pid);

    return 0;
}

static long ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    pid_t pid = current->pid;
    pcb* curr_proc = NULL;
    obj_info* info = NULL;
    unsigned char* type = NULL;
    unsigned char* order = NULL;
    search_obj* obj = NULL;
    __INT8_TYPE__ error = 0;
    long return_value = 0;

    while (!mutex_trylock(&pcb_mutex));
    curr_proc = pcb_list_Get(pid);
    mutex_unlock(&pcb_mutex);

    if (curr_proc == NULL) {
        printk(KERN_ERR "ioctl() - Process: %d has not Opened File\n", pid);
        return -EACCES;
    }

    type = (unsigned char*)kmalloc(sizeof(unsigned char), GFP_KERNEL);
    order = (unsigned char*)kmalloc(sizeof(unsigned char), GFP_KERNEL);
    obj = (search_obj*)kmalloc(sizeof(search_obj), GFP_KERNEL);
    info = obj_info_Create(0, 0, 0, 0, INF);

    switch (cmd) {
        case PB2_SET_TYPE:  {
                                if (copy_from_user(type, (unsigned char*)arg, sizeof(unsigned char))) {
                                    error = -ENOBUFS;
                                    break;
                                }
                                pcb_node_Reset(curr_proc);

                                switch (*type) {
                                    case NUM:   printk(KERN_NOTICE "ioctl() - Process: %d Set Object Type to NUM\n", pid);
                                                curr_proc->objType = NUM;
                                                break;
                                    case STR:   printk(KERN_NOTICE "ioctl() - Process: %d Set Object Type to STR\n", pid);
                                                curr_proc->objType = STR;
                                                break;
                                    default:    printk(KERN_ALERT "ioctl() - Unrecognized objType in Process: %d\n", pid);
                                                error = -EINVAL;
                                                break;
                                }
                                if (error < 0) {
                                    break;
                                }

                                break;
                            }
        case PB2_SET_ORDER: {
                                if (curr_proc->objType == NONE) {
                                    printk(KERN_ALERT "ioctl() - PB2_SET_TYPE not called by Process: %d\n", pid);
                                    error = -EACCES;
                                    break;
                                }
                                if (copy_from_user(order, (unsigned char*)arg, sizeof(unsigned char))) {
                                    error = -ENOBUFS;
                                    break;
                                }

                                switch (*order) {
                                    case INORDER:   printk(KERN_NOTICE "ioctl() - Process: %d Set Order Type to INORDER\n", pid);
                                                    curr_proc->orderType = INORDER;
                                                    break;

                                    case PREORDER:  printk(KERN_NOTICE "ioctl() - Process: %d Set Order Type to PREORDER\n", pid);
                                                    curr_proc->orderType = PREORDER;
                                                    break;

                                    case POSTORDER: printk(KERN_NOTICE "ioctl() - Process: %d Set Order Type to POSTORDER\n", pid);
                                                    curr_proc->orderType = POSTORDER;
                                                    break;

                                    default:        printk(KERN_ALERT "ioctl() - Unrecognized orderType in Process: %d\n", pid);
                                                    error = -EINVAL;
                                                    break;
                                }
                                if (error < 0) {
                                    break;
                                }

                                switch (curr_proc->objType) {
                                    case NUM:   printk(KERN_NOTICE "ioctl() - Process: %d Resetting NUM Output Cursor\n", pid);
                                                i_reset_cursor(&(curr_proc->i_top), curr_proc->i_root);
                                                break;
                                    case STR:   printk(KERN_NOTICE "ioctl() - Process: %d Resetting STR Output Cursor\n", pid);
                                                s_reset_cursor(&(curr_proc->s_top), curr_proc->s_root);
                                                break;
                                    default:    printk(KERN_ALERT "ioctl() - Unrecognized objType in Process: %d\n", pid);
                                                error = -EINVAL;
                                                break;
                                }
                                if (error < 0) {
                                    break;
                                }

                                break;
                            }
        case PB2_GET_INFO:  {
                                if (curr_proc->objType == NONE) {
                                    printk(KERN_ALERT "ioctl() - PB2_SET_TYPE not called by Process: %d\n", pid);
                                    error = -EACCES;
                                    break;
                                }

                                switch (curr_proc->objType) {
                                    case NUM:   printk(KERN_ALERT "ioctl() - Process: %d Getting NUM Tree Info\n", pid);
                                                return_value = i_bst_Info(curr_proc->i_root, info);
                                                break;
                                    case STR:   printk(KERN_ALERT "ioctl() - Process: %d Getting STR Tree Info\n", pid);
                                                return_value = s_bst_Info(curr_proc->s_root, info);
                                                break;
                                    default:    printk(KERN_ALERT "ioctl() - Unrecognized objType in Process: %d\n", pid);
                                                error = -EINVAL;
                                                break;
                                }

                                if (copy_to_user((obj_info*)arg, info, sizeof(obj_info))) {
                                    error = -ENOBUFS;
                                    break;
                                }

                                break;
                            }
        case PB2_GET_OBJ:   {
                                if (curr_proc->objType == NONE) {
                                    printk(KERN_ALERT "ioctl() - PB2_SET_TYPE not called by Process: %d\n", pid);
                                    error = -EACCES;
                                    break;
                                }
                                if (copy_from_user(obj, (search_obj*)arg, sizeof(search_obj))) {
                                    error = -ENOBUFS;
                                    break;
                                }

                                if (curr_proc->objType != obj->objtype) {
                                    error = -EINVAL;
                                    break;
                                }

                                obj->found = 0;
                                switch (curr_proc->objType) {

                                    case NUM:   printk(KERN_NOTICE "ioctl() - Searching %d in Process: %d\n", obj->int_obj, pid);
                                                if (i_bst_Search(curr_proc->i_root, obj->int_obj)) {
                                                    printk(KERN_NOTICE " - FOUND\n");
                                                    obj->found = 1;
                                                }
                                                break;

                                    case STR:   printk(KERN_NOTICE "ioctl() - Searching %s in Process: %d\n", obj->str, pid);
                                                if (s_bst_Search(curr_proc->s_root, obj->str)) {
                                                    printk(KERN_NOTICE " - FOUND\n");
                                                    obj->found = 1;
                                                }
                                                break;

                                    default:    printk(KERN_ALERT "ioctl() - Unrecognized objType in Process: %d\n", pid);
                                                error = -EINVAL;
                                                break;
                                }

                                if (copy_to_user((search_obj*)arg, obj, sizeof(search_obj))) {
                                    error = -ENOBUFS;
                                    break;
                                }
                                break;
                            }
        default:            printk(KERN_ALERT "ioctl() - Invalid Command in Process: %d\n", pid);
                            error = -EINVAL;
                            break;
    }
    kfree(type);
    kfree(order);
    kfree(info);
    kfree(obj);

    if (error < 0) {
        return error;
    }

    return return_value;
}

static ssize_t write(struct file *file, const char *buf, size_t count, loff_t *pos) {
    pid_t pid = current->pid;
    pcb* curr_proc = NULL;
    unsigned char* buffer = NULL;
    __INT8_TYPE__ error = 0;

    if (buf == NULL || count == 0) {
        return -EINVAL;
    }

    while (!mutex_trylock(&pcb_mutex));
    curr_proc = pcb_list_Get(pid);
    mutex_unlock(&pcb_mutex);

    if (curr_proc == NULL) {
        printk(KERN_ERR "write() - Process: %d has not Opened File\n", pid);
        return -EACCES;
    }

    buffer = (unsigned char*)kcalloc(MAX , sizeof(unsigned char), GFP_KERNEL);
    if (copy_from_user(buffer, buf, count)) {
        return -ENOBUFS;
    }

    switch (curr_proc->objType) {
        case NONE:  printk(KERN_ALERT "write() - PB2_SET_TYPE not called by Process: %d\n", pid);
                    error = -EACCES;
                    break;

        case NUM:   printk(KERN_NOTICE "write() - Process: %d Inserting NUM %d\n", pid, *((__INT32_TYPE__*)buffer));
                    i_bst_Insert(&(curr_proc->i_root), *((__INT32_TYPE__*)buffer)); /* Insert in Tree */
                    i_reset_cursor(&(curr_proc->i_top), curr_proc->i_root);         /* Reset the Stack Cursor */
                    break;

        case STR:   printk(KERN_NOTICE "write() - Process: %d Inserting STR %s\n", pid, (char*)buffer);
                    s_bst_Insert(&(curr_proc->s_root), buffer);                     /* Insert in Tree */
                    s_reset_cursor(&(curr_proc->s_top), curr_proc->s_root);         /* Reset the Stack Cursor */
                    break;

        default:    printk(KERN_ALERT "write() - Unrecognized objType in Process: %d\n", pid);
                    error = -EINVAL;
                    break;
    }
    kfree(buffer);

    if (error < 0) {
        return error;
    }
    return count;
}

static ssize_t read(struct file *file, char *buf, size_t count, loff_t *pos) {
    pid_t pid = current->pid;
    pcb* curr_proc = NULL;
    unsigned char* buffer = NULL;
    __INT8_TYPE__ error = 1;
    __INT32_TYPE__*  i_val = NULL;
    unsigned char* s_val = NULL;
    size_t bytes = 0;
    i_node* inode = NULL;
    s_node* snode = NULL;

    if (buf == NULL || count == 0) {
        return -EINVAL;
    }

    while (!mutex_trylock(&pcb_mutex));
    curr_proc = pcb_list_Get(pid);
    mutex_unlock(&pcb_mutex);

    if (curr_proc == NULL) {
        printk(KERN_ERR "read() - Process: %d has not Opened File\n", pid);
        return -EACCES;
    }

    switch (curr_proc->objType) {
        case NONE:  printk(KERN_ALERT "read() - PB2_SET_TYPE not called by Process: %d\n", pid);
                    error = -EACCES;
                    break;

        case NUM:   switch (curr_proc->orderType) {
                        case INORDER:   printk(KERN_NOTICE "read() - Process: %d Reading Next INORDER Node\n", pid);
                                        inode = i_bst_Inorder_Next(&(curr_proc->i_top));
                                        break;

                        case PREORDER:  printk(KERN_NOTICE "read() - Process: %d Reading Next PREORDER Node\n", pid);
                                        inode = i_bst_Preorder_Next(&(curr_proc->i_top));
                                        break;

                        case POSTORDER: printk(KERN_NOTICE "read() - Process: %d Reading Next POSTORDER Node\n", pid);
                                        inode = i_bst_Postorder_Next(&(curr_proc->i_top));
                                        break;

                        default:        printk(KERN_ALERT "read() - Unrecognized orderType in Process: %d\n", pid);
                                        error = -EINVAL;
                                        break;
                    }
                    if (error < 0) {
                        break;
                    }
                    if (inode == NULL) {
                        printk(KERN_WARNING "Process: %d has completed Traversal\n", pid);
                        error = 0;
                        break;
                    }
                    printk(KERN_NOTICE "read() - Process: %d has read %d\n", pid, inode->value);

                    i_val = (__INT32_TYPE__*)kmalloc(sizeof(__INT32_TYPE__), GFP_KERNEL);
                    *i_val = inode->value;
                    buffer = (unsigned char*)i_val;
                    bytes = sizeof(__INT32_TYPE__);
                    kfree(inode);
                    break;

        case STR:   switch (curr_proc->orderType) {
                        case INORDER:   printk(KERN_NOTICE "read() - Process: %d Reading Next INORDER Node\n", pid);
                                        snode = s_bst_Inorder_Next(&(curr_proc->s_top));
                                        break;

                        case PREORDER:  printk(KERN_NOTICE "read() - Process: %d Reading Next PREORDER Node\n", pid);
                                        snode = s_bst_Preorder_Next(&(curr_proc->s_top));
                                        break;

                        case POSTORDER: printk(KERN_NOTICE "read() - Process: %d Reading Next POSTORDER Node\n", pid);
                                        snode = s_bst_Postorder_Next(&(curr_proc->s_top));
                                        break;

                        default:        printk(KERN_ALERT "read() - Unrecognized orderType in Process: %d\n", pid);
                                        error = -EINVAL;
                                        break;
                    }
                    if (error < 0) {
                        break;
                    }
                    if (snode == NULL) {
                        printk(KERN_WARNING "Process: %d has completed Traversal\n", pid);
                        error = 0;
                        break;
                    }
                    printk(KERN_NOTICE "read() - Process: %d has read %s\n", pid, snode->value);

                    s_val = (unsigned char*)kcalloc(MAX, sizeof(unsigned char), GFP_KERNEL);
                    strcpy(s_val, snode->value);
                    buffer = (unsigned char*)s_val;
                    bytes = strlen(buffer);
                    kfree(snode);
                    break;

        default:    printk(KERN_ALERT "read() - Unrecognized objType in Process: %d\n", pid);
                    error = -EINVAL;
                    break;

    }
    if (error <= 0) {
        return error;
    }

    if (copy_to_user(buf, buffer, count)) {
        error = -ENOBUFS;
    }
    kfree(buffer);

    if (error < 0) {
        return error;
    }
    return bytes;
}

static int release(struct inode *inodep, struct file *filep) {
    pid_t pid = current->pid;
    pcb* curr_proc = NULL;

    while (!mutex_trylock(&pcb_mutex));
    curr_proc = pcb_list_Get(pid);
    mutex_unlock(&pcb_mutex);

    if (curr_proc == NULL) {
        printk(KERN_ERR "close() - Process: %d has not Opened File\n", pid);
        return -EACCES;
    }

    pcb_node_Reset(curr_proc);

    while (!mutex_trylock(&pcb_mutex));
    pcb_list_Delete(pid);
    mutex_unlock(&pcb_mutex);

    printk(KERN_NOTICE "close() - File Closed by Process: %d\n", pid);

    return 0;
}


static int load_module(void)
{
    struct proc_dir_entry *entry = proc_create("bst_store", 0, NULL, &file_ops);
    if(entry == NULL) {
        return -ENOENT;
    }

    file_ops.owner = THIS_MODULE;
    file_ops.write = write;
    file_ops.read = read;
    file_ops.open = open;
    file_ops.release = release;
    file_ops.unlocked_ioctl = ioctl;

    mutex_init(&pcb_mutex);
    printk(KERN_ALERT "LKM is ON\n");

    return 0;
}

static void release_module(void)
{
    remove_proc_entry("bst_store", NULL);

    mutex_destroy(&pcb_mutex);
    printk(KERN_ALERT "LKM is OFF\n");
}

module_init(load_module);
module_exit(release_module);
