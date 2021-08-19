# define sySkip 257
# define syRoutine 258
# define sySimpleRoutine 259
# define syCamelotRoutine 260
# define sySimpleProcedure 261
# define syProcedure 262
# define syFunction 263
# define sySubsystem 264
# define syKernel 265
# define syCamelot 266
# define syMsgType 267
# define syWaitTime 268
# define syNoWaitTime 269
# define syErrorProc 270
# define syServerPrefix 271
# define syUserPrefix 272
# define syRCSId 273
# define syImport 274
# define syUImport 275
# define sySImport 276
# define syIn 277
# define syOut 278
# define syInOut 279
# define syRequestPort 280
# define syReplyPort 281
# define syType 282
# define syArray 283
# define syStruct 284
# define syOf 285
# define syInTran 286
# define syOutTran 287
# define syDestructor 288
# define syCType 289
# define syCUserType 290
# define syCServerType 291
# define syColon 292
# define sySemi 293
# define syComma 294
# define syPlus 295
# define syMinus 296
# define syStar 297
# define syDiv 298
# define syLParen 299
# define syRParen 300
# define syEqual 301
# define syCaret 302
# define syTilde 303
# define syLAngle 304
# define syRAngle 305
# define syLBrack 306
# define syRBrack 307
# define syBar 308
# define syError 309
# define syNumber 310
# define sySymbolicType 311
# define syIdentifier 312
# define syString 313
# define syQString 314
# define syFileName 315
# define syIPCFlag 316

typedef union 
{
    u_int number;
    identifier_t identifier;
    string_t string;
    statement_kind_t statement_kind;
    ipc_type_t *type;
    struct
    {
	u_int innumber;		/* msg_type_name value, when sending */
	string_t instr;
	u_int outnumber;	/* msg_type_name value, when receiving */
	string_t outstr;
	u_int size;		/* 0 means there is no default size */
    } symtype;
    routine_t *routine;
    arg_kind_t direction;
    argument_t *argument;
    ipc_flags_t flag;
} YYSTYPE;
extern YYSTYPE yylval;
