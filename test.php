<?php
function C($key=null, $value=null, $default=null){
    return tp($key,$value,$default);
}
//C('a',true);
//C('a');
//C('a','axxx');
//var_dump('a');
//C('b','2222');
//var_dump(C('b',null,"b222"));
//C('db_name',["s" => "b"]);
//var_dump(C('db_name'));

//C("db.host","127.0.0.1");
//C("db.name","sb");
//C('t.city',["a","b"]);
//C('db.host',"localhost");
//C('db.host');
//C('t.city');
/*C("db.host","127.0.0.1");
var_dump(C("db.host"));
C("db.host","127.0.0.1");
C("name","sb");
exit;*/
//C('aaa',"a_value");
//C('host',"b_value");
//var_dump(C());
//exit;
//C("a","a1");
//C("a","a2");
//var_dump(C('a'));exit;
 //		var_dump(C("a.b",null,"default_a.b"));
 		
 //		C("a.b","a.b");
 //		var_dump(C("a.b"));
 //		exit('---------------');
//C("c","d");
//var_dump();
/*$a = array();
$a['mb'] = "mb";
exit;
C($a);*/
//C(array("mb"=> "mb"));
//var_dump(C("mb"));
//var_dump(C());

//exit;
define('THINK_PATH',"x");



$file = '/opt/webserver/php-5.5.17/bin/tp32/ThinkPHP/Conf/convention.php';
$a = include $file;
C($a);
var_dump(C());
exit;


$file = '/opt/webserver/php-5.5.17/bin/tp32/ThinkPHP/Conf/convention.php';

C(include $file);



//C(array('APP_USE_NAMESPACE' =>'xx'));
var_dump(C('APP_USE_NAMESPACE'));
$arr = array(
			'a' =>  false,   
			'b'  =>  array(),
       );
//C($arr);

