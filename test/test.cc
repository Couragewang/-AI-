#include <iostream>
#include <sstream>
#include <memory>
#include <string>
#include <json/json.h>

using namespace std;

int main()
{
    std::string str = "{\"Age\" : 26, \"Lang\" : \"c++\", \"Name\" : \"zhangsan\"}";
    cout << str << endl;

    JSONCPP_STRING errs;
    Json::Value root;
    Json::CharReaderBuilder rb;
    std::unique_ptr<Json::CharReader> const jsonReader(rb.newCharReader());
    bool res = jsonReader->parse(str.data(), str.data()+str.size(), &root, &errs);
    if(!res || !errs.empty()){
        cout << "jsoncpp parse error" << endl;
        return 1;
    }

    cout << root["Age"].asInt() << endl;
    cout << root["Lang"].asString() << endl;
    cout << root["Name"].asString() << endl;
}
















    //Json::Value root;
    //Json::StreamWriterBuilder wb;
    //std::ostringstream os;

    //root["name"] = "张三";
    //root["age"] = 19;
    //root["Lang"] = "C++";

    //std::unique_ptr<Json::StreamWriter> jw(wb.newStreamWriter());

    //jw->write(root, &os);
    //std::string result = os.str();

    //cout << result << endl;
//}
