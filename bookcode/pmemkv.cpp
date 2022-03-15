/*
 * pmemkv.cpp -- demonstrate a high-level key-value API for pmem
 */

#include <iostream>
#include <cassert>
#include <libpmemkv.hpp>

using namespace pmem::kv;
using std::cerr;
using std::cout;
using std::endl;
using std::string;

/*
 * for this example, create a 1 Gig file
 * called "/daxfs/kvfile"
 */
auto PATH = "/daxfs/kvfile";
const uint64_t SIZE = 1024 * 1024 * 1024;

/*
 * kvprint -- print a single key-value pair
 * 定义了一个 KV 对的输出函数，输出对应的数据到标准输出
 */
int kvprint(string_view k, string_view v) {
	cout << "key: "    << k.data() <<
		" value: " << v.data() << endl;
	return 0;
}

int main() {
   // 创建了一个 db 对象
	// start by creating the db object
	db *kv = new db();
	assert(kv != nullptr);

	// 设置相应的 config 信息，分别读取对应的宏定义
	// create the config information for
	// libpmemkv's open method
	config cfg;

	if (cfg.put_string("path", PATH) != status::OK) {
		cerr << pmemkv_errormsg() << endl;
		exit(1);
	}
	if (cfg.put_uint64("force_create", 1) != status::OK) {
		cerr << pmemkv_errormsg() << endl;
		exit(1);
	}
	if (cfg.put_uint64("size", SIZE) != status::OK) {
		cerr << pmemkv_errormsg() << endl;
		exit(1);
	}

  // 使用 cmap 引擎来打开该 db
	// open the key-value store, using the cmap engine
	if (kv->open("cmap", std::move(cfg)) != status::OK) {
		cerr << kv->errormsg() << endl;
		exit(1);
	}

	// 存入了一些数据
	// add some keys and values
	if (kv->put("key1", "value1") != status::OK) {
		cerr << kv->errormsg() << endl;
		exit(1);
	}
	if (kv->put("key2", "value2") != status::OK) {
		cerr << kv->errormsg() << endl;
		exit(1);
	}
	if (kv->put("key3", "value3") != status::OK) {
		cerr << kv->errormsg() << endl;
		exit(1);
	}

	// 遍历整个 db，然后输出对应的数据
	// iterate through the key-value store, printing them
	kv->get_all(kvprint);

	// stop the pmemkv engine
	delete kv;

	exit(0);
}