/*
 *	objc-actions.h
 *	Copyright 1988, NeXT, Inc.
 *	Author: s.naroff
 *
 *      Objective-C actions.
 */

/*** Public Interface (procedures) ***/

/* used by toplev.compile_file() */

void init_objc(), finish_objc();

/* used by c-parse.yyparse() */

tree start_class(enum tree_code code, tree className, tree superName);
tree continue_class(tree aClass);
tree finish_class(tree class);
void start_method_def(tree method);
void continue_method_def();
void finish_method_def();
void add_objc_decls();

tree is_ivar(tree decl_chain, tree ident);
int  is_public(tree anExpr, tree anIdentifier);
tree add_instance_variable(tree class, int isPublic,
                    tree delclarator, tree declspecs, tree width);
tree add_class_method(tree class, tree method);
tree add_instance_method(tree class, tree method);
tree get_super_receiver();
tree get_class_ivars(tree interface);
tree get_class_reference(tree interface);
tree get_static_reference(tree interface);

tree build_message_expr(tree mess);
tree build_selector_expr(tree mess);
tree build_ivar_reference(tree mess);
tree build_keyword_decl(tree sel_name, tree arg_type, tree arg_name);
tree build_method_decl(enum tree_code code, tree ret_type, 
			tree keyword_selector, tree add_args);

/* the following routines are used to implement statically typed objects */

tree lookup_interface(tree anIdentifier); /* `c-parse.yylex()' */
int  objc_comptypes(tree lhs, tree rhs); /* `c-typeck.comptypes()' */
void objc_check_decl(tree aDecl); /* `toplev.rest_of_decl_compilation()' */

/* NeXT extensions */

tree build_encode_expr(tree type); 

/* used by toplev.report_error_function() */

char *compiling_a_method();	

/* used by toplev.rest_of_compilation() */

void genPrototype(FILE *fp, tree decl);		

/*** Public Interface (data) ***/

;
