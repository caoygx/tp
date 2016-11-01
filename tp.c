/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2014 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "main/SAPI.h"

#include "ext/standard/info.h"
#include "ext/standard/php_string.h"
#include "ext/standard/php_var.h"
#include "ext/standard/php_array.h"
#include "Zend/zend_globals_macros.h"
#include "php_tp.h"

ZEND_DECLARE_MODULE_GLOBALS(tp)

/* True global resources - no need for thread safety here */
static int le_tp;
static zval *config;
//static HashTable *arr;

/* {{{ tp_functions[]
 *
 * Every user visible function must have an entry in tp_functions[].
 */
const zend_function_entry tp_functions[] = {
    PHP_FE(tp, NULL)       /* For testing, remove later. */
    PHP_FE_END  /* Must be the last line in tp_functions[] */
};
/* }}} */


#ifdef COMPILE_DL_TP
ZEND_GET_MODULE(tp)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("tp.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_TP_Globals, TP_Globals)
    STD_PHP_INI_ENTRY("tp.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_TP_Globals, TP_Globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_tp_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_tp_init_globals(zend_TP_Globals *TP_Globals)
{
    TP_Globals->global_value = 0;
    TP_Globals->global_string = NULL;
}
*/
/* }}} */


/*
*  loader_import首先将PHP源文件编译成op_array，然后依次执行op_array中的opcode
*/
int loader_import(char *path, int len TSRMLS_DC,zval **result) {
    zend_file_handle file_handle;
    zend_op_array   *op_array;
    char realpath[MAXPATHLEN];

    if (!VCWD_REALPATH(path, realpath)) {
        return 0;
    }

    file_handle.filename = path;
    file_handle.free_filename = 0;
    file_handle.type = ZEND_HANDLE_FILENAME;
    file_handle.opened_path = NULL;
    file_handle.handle.fp = NULL;

    //调用zend API编译源文件
    op_array = zend_compile_file(&file_handle, ZEND_INCLUDE TSRMLS_CC);

    if (op_array && file_handle.handle.stream.handle) {
        int dummy = 1;

        if (!file_handle.opened_path) {
            file_handle.opened_path = path;
        }

        //将源文件注册到执行期间的全局变量(EG)的include_files列表中，这样就标记了源文件已经包含过了
        zend_hash_add(&EG(included_files), file_handle.opened_path, strlen(file_handle.opened_path)+1, (void *)&dummy,
                sizeof(int), NULL);
    }
    zend_destroy_file_handle(&file_handle TSRMLS_CC);

    //开始执行op_array
    if (op_array) {
//        zval *result = NULL;
        //保存原来的执行环境，包括active_op_array,opline_ptr等
        zval ** __old_return_value_pp   =  EG(return_value_ptr_ptr);
        zend_op ** __old_opline_ptr     = EG(opline_ptr); 
        zend_op_array * __old_op_array  = EG(active_op_array);
        //保存环境完成后，初始化本次执行环境，替换op_array
        EG(return_value_ptr_ptr) = result;
        EG(active_op_array)      = op_array;

#if ((PHP_MAJOR_VERSION == 5) && (PHP_MINOR_VERSION > 2)) || (PHP_MAJOR_VERSION > 5)
        if (!EG(active_symbol_table)) {
            zend_rebuild_symbol_table(TSRMLS_C);
        }
#endif
        //调用zend API执行源文件的op_array
        zend_execute(op_array TSRMLS_CC);
        //op_array执行完成后销毁，要不然就要内存泄露了，哈哈
        destroy_op_array(op_array TSRMLS_CC);
        efree(op_array);
        //通过检查执行期间的全局变量(EG)的exception是否被标记来确定是否有异常
        if (!EG(exception)) {
            if (EG(return_value_ptr_ptr) && *EG(return_value_ptr_ptr)) {
    //php_var_dump(result,1);
                Z_ADDREF_P(*result);
                zval_ptr_dtor(EG(return_value_ptr_ptr));
            }
        }
        //ok,执行到这里说明源文件的op_array已经执行完成了，我们要恢复原来的执行环境了
        EG(return_value_ptr_ptr) = __old_return_value_pp;
        EG(opline_ptr)           = __old_opline_ptr; 
        EG(active_op_array)      = __old_op_array; 

        return 1;
    }
    return 0;
}

int php_sample_print_zval(zval **val TSRMLS_DC)
{
    //重新copy一个zval，防止破坏原数据
    zval tmpcopy = **val;
    zval_copy_ctor(&tmpcopy);
    
    //转换为字符串
    INIT_PZVAL(&tmpcopy);
    convert_to_string(&tmpcopy);
   
    //开始输出
    php_printf("The value is: ");
    PHPWRITE(Z_STRVAL(tmpcopy), Z_STRLEN(tmpcopy));
    php_printf("\n");
    
    //毁尸灭迹
    zval_dtor(&tmpcopy);
    
    //返回，继续遍历下一个～
    return ZEND_HASH_APPLY_KEEP;
}





int c_get_all(zval **r_value){
    //key为空获取所有

            zval *r_config = (zval *)pemalloc(sizeof(zval), 1);
               INIT_PZVAL(r_config);
           Z_TYPE_P(r_config) = IS_ARRAY;
           r_config->value.ht = TP_G(config);

    //      ZVAL_COPY_VALUE(return_value,config);
    //        zend_hash_apply(TP_G(config), php_sample_print_zval TSRMLS_CC);
    //        return 1;
     //      RETURN_ZVAL(r_config,1,0);
           *r_value = r_config;
         return;
}

int c_set_string(char *name, zval *value){
            Z_ADDREF_P(value); //怀疑value引用增加后，执行结束后并没有释放掉
            zend_hash_update(TP_G(config), name, strlen(name)+1, &value, sizeof(zval *), NULL);
            //Z_DELREF_P(value); //不执行则报内存泄漏，执行则get时获取不到值
             return 1;
}


int c_get_string(char *name, zval *value, zval *default_value, zval **r_value){
        //单字符串获取
       if(value == NULL || Z_TYPE_P(value) == IS_NULL){//get

           zval **ppdata;
           if(zend_hash_find(TP_G(config),name,strlen(name)+1,(void **)&ppdata) != FAILURE)
           {
               *r_value = *ppdata;
               return 1;
           }
           if(default_value != NULL && Z_TYPE_P(default_value) != IS_NULL  ){
               //ZVAL_COPY_VALUE(return_value,default_value);
               *r_value = default_value;
               return 1;
           }
           return 0;
       }
       return 1;

}

int c_set_dot_string(char *firstKey, char *secondKey,  zval *value){
     zval *tmp;
                MAKE_STD_ZVAL(tmp);
                zval **ppzval;

                if(zend_hash_find(TP_G(config),firstKey,strlen(firstKey)+1,(void **)&ppzval) != FAILURE){
                    ZVAL_COPY_VALUE(tmp,value);
                    zend_hash_update(Z_ARRVAL_PP(ppzval),secondKey,strlen(secondKey)+1,(void **)&tmp,sizeof(zval *),NULL);
                    return 1;
    //              php_printf("%s\n","find");
            //      zval_ptr_dtor(ppzval);
                }
                else{
                    zval *firstArray;
                    MAKE_STD_ZVAL(firstArray);
                    array_init(firstArray);
                    //add_assoc_string(firstArray,secondKey,"api",1);
                    ZVAL_COPY_VALUE(tmp,value);
                    add_assoc_zval(firstArray,secondKey,tmp);
                    //add_assoc_zval(config,firstKey,firstArray);
                    zend_hash_update(TP_G(config),firstKey,strlen(firstKey)+1,(void **)&firstArray,sizeof(zval *),NULL);
                    return 1;
                }
}

int c_get_dot_string(char *firstKey, char *secondKey, zval *value, zval *default_value, zval **r_value){
    zval **ppzval;
                if(zend_hash_find(TP_G(config),firstKey,strlen(firstKey)+1,(void **)&ppzval) != FAILURE){
                    zval **secondValue=NULL;
                    if(zend_hash_find(Z_ARRVAL_P(*ppzval),secondKey,strlen(secondKey)+1,(void **)&secondValue) != FAILURE){
                        *r_value = *secondValue;
                        return 1;
    //                    RETURN_ZVAL(*secondValue,1,0);
                    }
            //      zval_ptr_dtor(ppzval);
                }
                if(default_value != NULL && Z_TYPE_P(default_value) != IS_NULL  ){
    //              php_printf("no find\n");
      //              ZVAL_COPY_VALUE(return_value,default_value);
                    *r_value = default_value;
                    return 1;
                }else{
                    return 0;
    //                RETURN_NULL();
                }
}




int c_set_array(zval *key){
    zend_fcall_info fci;
    zend_fcall_info_cache fcc;
    zval *retval;
    zval *handle = NULL;
    zval function_name;
    zval **argv[2];

    ZVAL_STRING(&function_name, "array_change_key_case", 1);
    MAKE_STD_ZVAL(handle);

    zval *type;
    MAKE_STD_ZVAL(type);
    ZVAL_LONG(type,1);
    argv[0] = &key;
    argv[1] = &type;
    fci.size = sizeof(fci);
    fci.function_table = EG(function_table);
    fci.function_name = &function_name;
    fci.symbol_table = NULL;
    fci.object_ptr = NULL;
    fci.retval_ptr_ptr = &retval;
    fci.param_count = 2;
    fci.params = argv;
    fci.no_separation = 0;


    //php_var_dump(&key,1);
    if (zend_call_function(&fci, NULL TSRMLS_CC) == FAILURE) {
        php_printf("err");
    }
    else {
        //php_var_dump(&retval,1);
        php_array_merge(TP_G(config),Z_ARRVAL_PP(&retval), 0 TSRMLS_CC);
        //zval_ptr_dtor(&key);
    }
    //      zval_ptr_dtor(&retval);
    zval_ptr_dtor(&handle);
    zval_ptr_dtor(&type);
    zval_dtor(&function_name);
    zval_ptr_dtor(*argv);
    return 1;
}




/** {{{ static void yaf_config_cache_dtor(yaf_config_cache **cache)
 *  * 销毁配置文件的缓存
 *   */
 /*static void thinkphp_dtor(yaf_config_cache **cache) {
        if (*cache) {
                    zend_hash_destroy((*cache)->data);
                    pefree((*cache)->data, 1);
                    pefree(*cache, 1);
                }
 }
 */
 /* }}} */

int init_config(){


    //                add_next_index_string(TP_G(config),"request",1);
    //   MAKE_STD_ZVAL(config);
    //    array_init(config);

    if(TP_G(config) == NULL || !zend_hash_num_elements(TP_G(config)) ){
        char *path = "/home/wwwroot/redlight/framework/ThinkPHP/Conf/convention.php";
        zval *ret_config1;
        loader_import(path,strlen(path),&ret_config1);
//             php_var_dump(&ret_config1,1);
c_set_array(ret_config1);

        char *path2 = "/home/wwwroot/redlight/Application/Common/Conf/config.php";
        zval *ret_config;
        loader_import(path2,strlen(path),&ret_config);
 //            php_var_dump(&ret_config,1);
        //    zval_ptr_dtor(&ret_config);
        //     zval *ret = NULL;
        //
       c_set_array(ret_config);
//        tp(ret_config,NULL,NULL,NULL);

    }
    //if(tp(ret_config,NULL,NULL,NULL)){
    //     php_var_dump(&config,1);
    //        if(ret != NULL){
    //  }
    //
    return 1;
}

PHP_GINIT_FUNCTION(tp)
{
        tp_globals->config = NULL;
}

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(tp)
{
    /* If you have INI entries, uncomment these lines 
    REGISTER_INI_ENTRIES();
    */

    TP_G(config) = (HashTable *)pemalloc(sizeof(HashTable), 1);
    zend_hash_init(TP_G(config), 128, NULL, NULL, 1);

//REGISTER_LONG_CONSTANT("ONLINE",0, CONST_CS | CONST_PERSISTENT);
//REGISTER_STRING_CONSTANT("THINK_PATH","/home/wwwroot/redlight/framework/ThinkPHP/", CONST_CS | CONST_PERSISTENT);
//REGISTER_STRING_CONSTANT("TEMP_PATH","/home/wwwroot/redlight/Runtime/Temp/", CONST_CS | CONST_PERSISTENT);


//       MAKE_STD_ZVAL(TP_G(config));
  //  array_init(TP_G(config));

    //            add_assoc_string(TP_G(config),"m","m",1);
/*   le_tp = 100;
    php_printf("i:%d",le_tp);
    printf("c M init");
    php_printf("M INIT");
    TP_G(arr) = (HashTable *)pemalloc(sizeof(HashTable), 1);
    zend_hash_init(TP_G(arr), 8, NULL, NULL, 1);
   zval *v = NULL;
                MAKE_STD_ZVAL(v);
          ZVAL_STRING(v, "sbxx2b", 1);
    zend_hash_update(TP_G(arr), "m", strlen("m")+1, &v, sizeof(zval *), NULL);
*/


//  zval *firstArray;
 // zval *tmp;
  //zval *value;
    //            MAKE_STD_ZVAL(firstArray);
//                add_assoc_string(arr,"m","m",1);
  //    add_next_index_string(arr,"request",1);
      //          ZVAL_COPY_VALUE(tmp,value);
//    php_var_dump(arr,1);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(tp)
{
    /* uncomment this line if you have INI entries
    UNREGISTER_INI_ENTRIES();
    */
   // zval_ptr_dtor(&config);
   // config =NULL;
//    if(config != NULL){
//        php_var_dump(&config,1);
  //  }
//    php_var_dump(&TP_G(config),1);
//    php_printf("mshut");
    //
    
    zend_hash_destroy(TP_G(config));                                                                                                                                  
    pefree(TP_G(config), 1); 

    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(tp)
{
/*   le_tp ++;
//    php_var_dump(&TP_G(config),1);
  php_printf("i:%d",le_tp);  
    zval *v = NULL;
                MAKE_STD_ZVAL(v);
          ZVAL_STRING(v, "r", 1);
    ulong nextid = zend_hash_next_free_element(TP_G(arr));
    zend_hash_index_update(TP_G(arr), nextid, &v, sizeof(zval *), NULL);

    php_printf("key is %d",nextid);
    zend_hash_update(TP_G(arr), "r", strlen("r")+1, &v, sizeof(zval *), NULL);
*/
//   init_config();
    //    MAKE_STD_ZVAL(config);
    //    array_init(config);
    return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(tp)
{
 
//    php_printf("rshutdown:");
//    php_var_dump(&TP_G(config),1);
    //php_var_dump(&config,1);

    return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(tp)
{
    php_info_print_table_start();
    php_info_print_table_header(2, "tp support", "enabled");
    php_info_print_table_end();

    /* Remove comments if you have entries in php.ini
    DISPLAY_INI_ENTRIES();
    */
}
/* }}} */




/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_tp_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(tp)
{
    zval *key=NULL;
    zval *value=NULL;
    zval *default_value=NULL;
    zval *ret_value=NULL;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "|zzz", &key, &value, &default_value) == FAILURE) {
        php_printf("param error");
        RETURN_NULL();  
    }

    if (key == NULL || Z_TYPE_P(key) == IS_NULL) {
        c_get_all(&ret_value);
        if(ret_value){
               RETURN_ZVAL(ret_value,1,0)
        }
        else{
               RETURN_NULL();
        }

        //RETURN_ZVAL(ret_value,1,0);
    }

    //单字符串获取
    if (Z_TYPE_P(key) == IS_STRING) {
        char *name;
//        name = (char *) emalloc(Z_STRLEN_P(key)+1);
  //      strcpy(name, Z_STRVAL_P(key));
        name = estrndup(Z_STRVAL_P(key), Z_STRLEN_P(key));
//                name = Z_STRVAL_P(key);
        name = php_strtoupper(name, strlen(name));
        if (!strstr(name, ".")) {
            if (value == NULL || Z_TYPE_P(value) == IS_NULL) {    //get
                c_get_string(name,NULL,default_value,&ret_value);
                if(ret_value){
                    RETURN_ZVAL(ret_value,1,0)
                }
                else{
                    RETURN_NULL();
                }
           //     php_var_dump(ret_value,1);
            } else {    //set
                c_set_string(name,value);
        //        zend_hash_apply(TP_G(config), php_sample_print_zval TSRMLS_CC);
            }
            efree(name);
        }
        else
        {
                //二维数据设置和获取
                    char *firstKey = NULL;
                    char *secondKey= NULL;
                    char *nameCopy = NULL;
                    //nameCopy= (char *)malloc(strlen(name)+1);
        //            strcpy(nameCopy,name);
                    nameCopy = estrndup(name, strlen(name));
            //        php_printf("copy=%s\n",nameCopy);
                    firstKey= strtok(nameCopy,".");
                    secondKey= strtok(NULL,".");
                    if (value == NULL || Z_TYPE_P(value) == IS_NULL) {    //get
                        c_get_dot_string(firstKey,secondKey,NULL,default_value,&ret_value);
                    if(ret_value){
                        RETURN_ZVAL(ret_value,1,0)
                    }
                else{
                    RETURN_NULL();
                }
                    } else {    //set
                        c_set_dot_string(firstKey, secondKey,value);
                    }
                    efree(name);
                    efree(nameCopy);

        }

    }

     if(Z_TYPE_P(key) == IS_ARRAY){
        c_set_array(key);
     }
        RETURN_NULL();  
}

/*
//    char *path = "/home/wwwroot/redlight/framework/ThinkPHP/Conf/convention.php";
//    char *path = "/root/lnmp1.3-full/src/php-5.5.17/ext/tp/a.php";
    zval *ret_config;
// loader_import(path,strlen(path),&ret_config);
//            php_var_dump(&ret_config,1);
//    zval_ptr_dtor(&ret_config);
    zval *ret = NULL;
    if(tp(key,value,default_value,&ret)){
//           php_var_dump(&config,1);
        if(ret != NULL){
            //php_var_dump(&config,1);
            RETURN_ZVAL(ret,1,0);
        }
        RETURN_NULL();  
    }else{
    
    }


*/

/* }}} */


/* {{{ tp_module_entry
 */
zend_module_entry tp_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    "tp",
    tp_functions,
    PHP_MINIT(tp),
    PHP_MSHUTDOWN(tp),
    PHP_RINIT(tp),      /* Replace with NULL if there's nothing to do at request start */
    PHP_RSHUTDOWN(tp),  /* Replace with NULL if there's nothing to do at request end */
    PHP_MINFO(tp),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_TP_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/
/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */



