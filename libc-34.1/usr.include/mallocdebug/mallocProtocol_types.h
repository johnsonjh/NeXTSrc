
#import <mach.h>

typedef char *data_t;
typedef int retval_t;

enum status
{
  status_ok,
  status_bad_left,
  status_bad_right,
};

typedef struct {
  void *addr;
  void *pc;
  void *pc2;
  int size;
  int mark;
  enum status status;
} MallocData;

typedef enum
{
  marked_nodes = 0,
  all_nodes = 1,
  smashed_nodes = 2
} NodeType;


