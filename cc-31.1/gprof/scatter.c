/* scatter.c */

#include "gprof.h"
#include <string.h>
#include <stdio.h>
#include <libc.h>

#define MAXINT 0x7fff

typedef struct Edgestruct { 
  struct Edgestruct *next1, *next2, *prev1, *prev2, **pqp;
  short v1, v2, w;
} Edge;

typedef struct { 
  Edge *next1, *next2; 
  short parent, left, right; 
} TreeNode;

#define NONE -1
#define SEEN -2

void pqinsert(Edge *edge);
Edge *pqremove();
void pqdelete(Edge *edge);
static void upheap(int k);
static void downheap(int k);

FILE *gmon, *callf, *treefile;
TreeNode *tree;
int n_nodes;
Edge **pq;
int n_pq;
char *whatsloaded;
Edge *free_edges = NULL;
Edge pqzero;

void do_what() {
  struct file *afile;
  char *start, *stop, *dest, *whatsloadedp, ar_name[16], *namep;
  for (afile = files; afile < &files[n_files]; afile++) {
    if (namep = strrchr(afile->name, '/')) namep++; 
    else namep = afile->name;
    strncpy(ar_name, namep, 15); ar_name[15] = '\0';
    whatsloadedp = whatsloaded;
    while (start = stop = strstr(whatsloadedp, ar_name)) {
      if ((start == whatsloaded) || (*(start-1) == '(') || (*(start-1) == '/')) {
	while ((*start != '\n') && (start >= whatsloaded)) start--;
	while ((*stop != '\n') && (*stop != ')')) stop++;
	afile->what_name = dest = malloc(stop - start);
	for (start++; start < stop; start++) {
	  *(dest++) = (*start == '(') ? ':' : *start;
	}
	*dest = '\0';
	break;
      }
      whatsloadedp = start + 1;
    }
  }
} 

char *find_file(unsigned long pc) {
  struct file *afile;
  for (afile = files; afile < &files[n_files]; afile++) {
    if ((pc >= afile->firstpc) && (pc < afile->lastpc)) {
      return afile->what_name;
    }
  }
  return NULL;
}

int printp(nltype *node) {
  return !
    ((node->ncall == 0) &&
     (node->selfcalls == 0) &&
     (node->propself == 0) &&
     (node->propchild == 0));
}

/*
  void indent_node(int node, int level)
  for (i = 0; i < level; i++) fprintf(treefile, "  ");
  fprintf(treefile, "%d %s\n", node, (node < npe - nl) ? nl[node].name : "");
  */

void print_node(int node) {
  if ((node == NONE) || (tree[node].parent == SEEN)) return;
  if (node < (npe - nl)) {
    nltype *nlp = &nl[node];
    if (printp(nlp)) {
      char *file = find_file(nlp->value);
      fprintf(gmon, "%s:%s\n", file ? file : "", nlp->name);
    }
  }
  tree[node].parent = SEEN;
  print_node(tree[node].left);
  print_node(tree[node].right);
}

void print_nl() {
  nltype *nlp;
  for (nlp = npe-1; nlp >= nl; nlp--) {
    if (printp(nlp)) {
      char *file = find_file(nlp->value);
      fprintf(callf, "%s:%s\n", file ? file : "", nlp->name);
    }
  }
}

void print_tree() {
  int i;
  for (i = n_nodes-1; i >= 0; i--) {
    if (tree[i].parent != SEEN) print_node(i);
  }
}

Edge *find_edge(int v1, int v2, int w) {
  Edge *edge;
  for (edge = tree[v1].next1; edge; edge = edge->next1) {
    if (edge->v2 == v2) {
      edge->w += w;
      return edge;
    }
  }
  return NULL;
} 

Edge *make_edge(int v1, int v2, int w) {
  Edge *edge;
  if (free_edges) { edge = free_edges; free_edges = free_edges->next1; }
  else edge = (Edge *)malloc(sizeof(Edge));
  edge->v1 = v1; edge->v2 = v2; edge->w = w;

  edge->next1 = tree[v1].next1;
  if (edge->next1) edge->next1->prev1 = edge;
  tree[v1].next1 = edge;
  edge->prev1 = NULL;
  
  edge->next2 = tree[v2].next2; 
  if (edge->next2) edge->next2->prev2 = edge;
  tree[v2].next2 = edge;
  edge->prev2 = NULL;

  edge->pqp = NULL;
  return edge;
}

void free_edge(Edge *edge) {

  if (edge->prev1) edge->prev1->next1 = edge->next1;
  else tree[edge->v1].next1 = edge->next1;

  if (edge->next1) edge->next1->prev1 = edge->prev1;

  if (edge->prev2) edge->prev2->next2 = edge->next2;
  else tree[edge->v2].next2 = edge->next2;

  if (edge->next2) edge->next2->prev2 = edge->prev2;

  edge->next1 = free_edges;
  free_edges = edge;
}

int compare_file (struct file *x, struct file *y) {
  if (x->firstpc < y->firstpc) return -1;
  else if (x->firstpc > y->firstpc) return 1;
  else return 0;
}

int compare_nl (nltype *x, nltype *y) {
  if (x->ncall < y->ncall) return -1;
  else if (x->ncall > y->ncall) return 1;
  else return 0;
}

void enum_arcs(void(*proc)(arctype *arc, arctype *backarc)) {
  nltype *nlp;
  arctype *arcp, *backarc;
  for ( nlp = nl ; nlp < npe ; nlp++ ) {
    for ( arcp = nlp -> children ; arcp ; arcp = arcp -> arc_childlist ) {
      for (backarc = arcp->arc_childp->children; backarc; backarc = backarc->arc_childlist) {
	if (backarc->arc_childp == nlp) {
	  if (nlp < arcp->arc_childp) proc(arcp, backarc);
	  goto skip;
	}
      }
      proc(arcp, NULL);
    skip: continue;
    }
  }
}

int most_edges;
void most_edges_proc(arctype *arc, arctype *backarc) { 
  most_edges++; 
}

void enum_edges(int v, int(*proc_e1)(Edge *), int(*proc_e2)(Edge *)) {
  Edge *edge;
  Edge *next1, *next2;
  edge = tree[v].next1; 
  while (edge) {
    next1 = edge->next1; 
    if (proc_e1(edge)) free_edge(edge);
    edge = next1;
  }
  edge = tree[v].next2; 
  while (edge) {
    next2 = edge->next2;
    if (proc_e2(edge)) free_edge(edge);
    edge = next2;
  }
}

int make_edge_e1(Edge *edge) {
  if (edge->pqp) {
    pqdelete(edge); 
    make_edge(n_nodes, edge->v2, edge->w);
    return 1;
  } else {
    return 0;
  }
}

int make_edge_e2(Edge *edge) {
  if (edge->pqp) {
    pqdelete(edge);
    make_edge(n_nodes, edge->v1, edge->w);
    return 1;
  } else {
    return 0;
  }
}

int find_edge_e1(Edge *edge) {
  if (edge->pqp) {
    pqdelete(edge); 
    if (!find_edge(n_nodes, edge->v2, edge->w))
      make_edge(n_nodes, edge->v2, edge->w);
    return 1;
  } else {
    return 0;
  }
}

int find_edge_e2(Edge *edge) {
  if (edge->pqp) {
    pqdelete(edge);
    if (!find_edge(n_nodes, edge->v1, edge->w))
      make_edge(n_nodes, edge->v1, edge->w);
    return 1;
  } else {
    return 0;
  }
}

int pqinsert_e1(Edge *edge) {
  if (!edge->pqp) pqinsert(edge);
  return 0;
}

void main_loop() {
  Edge *edge;
  
  while (n_pq > 0) {
    edge = pqremove();
    
    /* collapse endpoints into combined node */
    tree[n_nodes].parent = NONE;
    tree[n_nodes].left = edge->v1;
    tree[n_nodes].right = edge->v2;
    tree[edge->v1].parent = n_nodes;
    tree[edge->v2].parent = n_nodes;
    if (n_pq == 0) break;
    
    /* create edges for combined node */
    enum_edges(edge->v1, make_edge_e1, make_edge_e2);
    enum_edges(edge->v2, find_edge_e1, find_edge_e2);
    enum_edges(n_nodes, pqinsert_e1, NULL);
    
    if ((n_nodes++ % 50) == 0) putc('.', stderr);
  }
  putc(';', stderr); putc('\n', stderr);
}

void pqinsert_proc(arctype *arc, arctype *backarc) {
  pqinsert(make_edge(arc->arc_parentp - nl, 
		     arc->arc_childp - nl, 
		     (backarc) ? (arc->arc_count + backarc->arc_count) : (arc->arc_count)));
}

void scatter() {
  int max_nodes = 2 * (npe - nl);
  TreeNode *tnode;
  tree = (TreeNode *)malloc(max_nodes * sizeof(TreeNode));
  n_nodes = npe - nl;
  for (tnode = tree; tnode < &tree[max_nodes]; tnode++) {
    tnode->next1 = tnode->next2 = NULL;
    tnode->parent = tnode->left = tnode->right = NONE;
  }
  most_edges = 1;
  enum_arcs(most_edges_proc);
  pq = (Edge **)malloc(most_edges * sizeof(Edge *));
  n_pq = 0;
  pq[0] = &pqzero; pqzero.w = MAXINT;
  
  enum_arcs(pqinsert_proc); /* create undirected graph */
  main_loop();
  print_tree();
  qsort(nl, npe - nl, sizeof(nltype),  
	(int(*)(const void *, const void *))compare_nl);
  print_nl();
}

void printscatter() {
  int what_fd;
  if ((gmon = fopen("gmon.order", "w")) == NULL) {
    perror("gmon.order");
    done();
  }
  if ((callf = fopen("callf.order", "w")) == NULL) {
    perror("callf.order");
    done();
  }
  /*
  if ((treefile = fopen("gmon.tree", "w")) == NULL) {
    perror("gmon.tree");
    done();
  }
  */
  if ((what_fd = open("whatsloaded", O_RDONLY)) >= 0) {
    struct stat buf;
    fstat(what_fd, &buf);
    map_fd(what_fd, 0, &whatsloaded, TRUE, buf.st_size);
    do_what();
  }
  qsort(files, n_files, sizeof(struct file), 
	(int(*)(const void *, const void *))compare_file);
  scatter();
  fclose(gmon);
  fclose(callf);
  if (what_fd >= 0) close(what_fd);
}

#ifdef DEBUG
void ugh() {
}

static void print_pq() {
  int i; Edge *edge;
  for (i = 1; i <= n_pq; i++) {
    edge = pq[i];
    if (edge->pqp != &pq[i]) ugh();
    fprintf(stderr, "%4d %4d %4d\n", i, edge->v1, edge->v2);
  }
}
#endif

static void upheap(int k) {
  Edge *v;
  v = pq[k];
  while (pq[k/2]->w <= v->w) { 
    pq[k] = pq[k/2]; 
    pq[k]->pqp = &pq[k];
    k = k/2; }
  pq[k] = v; 
  pq[k]->pqp = &pq[k];
}

static void downheap(int k) {
  Edge *v;
  int j;
  v = pq[k];
  while (k <= n_pq/2) {
    j = 2*k;
    if (j < n_pq) if (pq[j]->w < pq[j+1]->w) j++;
    if (v->w >= pq[j]->w) break;
    pq[k] = pq[j];
    pq[k]->pqp = &pq[k];
    k = j;
  }
  pq[k] = v;
  pq[k]->pqp = &pq[k];
}

static void pqinsert(Edge *v) {
#ifdef DEBUG
  printf("pqinsert %d	%x  %d  %d\n", n_pq, v, v->v1, v->v2);
#endif
  pq[++n_pq] = v;
  upheap(n_pq);
}

static Edge *pqremove() {
  Edge *v = pq[1];
#ifdef DEBUG
  printf("pqremove %d	%x  %d  %d\n", n_pq, v, v->v1, v->v2);
#endif
  pq[1]->pqp = NULL;
  pq[1] = pq[n_pq--];
  downheap(1);
  return v;
}

static void pqdelete(Edge *v) {
  Edge **peeq = v->pqp;
#ifdef DEBUG
  printf("pqdelete %d	%x  %d  %d\n", n_pq, v, v->v1, v->v2);
#endif
  v->pqp = NULL;
  if (n_pq == 0) return;
  *peeq = pq[n_pq--];
  upheap(peeq - pq);
  downheap(peeq - pq);
}

