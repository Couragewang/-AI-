#ifndef _JARVIS_HPP_
#define _JARVIS_HPP_

#include <iostream>
#include <map>
#include <unordered_map>
#include <stdio.h>
#include <string>
#include <sstream>
#include <memory>
#include <json/json.h>
#include "speech.h"
#include "base/http.h"

#define VOICE_PATH "voice"
#define SPEECH_ASR "asr.wav"
#define CMD_ETC "command.etc"
#define SIZE 1024

using namespace std;

class TuLing{
    private:
        string url = "http://openapi.tuling123.com/openapi/api/v2";
        string apiKey = "a611cf556eaf472eb5e89037017f090b";
        string userId = "1";
        aip::HttpClient client;

    public:
        TuLing(){
        }
        string ResponsePickup(std::string &str)
        {
            JSONCPP_STRING errs;
            Json::Value root;
            Json::CharReaderBuilder rb;
            std::unique_ptr<Json::CharReader> const jsonReader(rb.newCharReader());
            bool res = jsonReader->parse(str.data(), str.data()+str.size(), &root, &errs);
            if(!res || !errs.empty()){
                cout << "jsoncpp parse error" << endl;
                return errs;
            }
            Json::Value results = root["results"];
            Json::Value values  = results[0]["values"];
            return values["text"].asString();
        }
        string Chat(string message)
        {
            Json::Value root;
            root["reqType"] = 0;
            Json::Value inputText; //bug?
            Json::Value text;
            text["text"] = message;
            inputText["inputText"] = text;

            root["perception"] = inputText;

            Json::Value user;
            user["apiKey"] = apiKey;
            user["userId"] = userId;

            root["userInfo"] = user;

            Json::StreamWriterBuilder wb;
            std::ostringstream os;

            std::unique_ptr<Json::StreamWriter> jsonwriter(wb.newStreamWriter());
            jsonwriter->write(root, &os);
            
            string body = os.str(); //有了json串,接下来？
            //http request!
            string response;
            int code = client.post(url, nullptr, body, nullptr, &response);
            if(code != CURLcode::CURLE_OK){
                cout << "http request error!" << endl;
                return "";
            }
            return ResponsePickup(response);
        }
        ~TuLing(){
        }
};

class Jarvis{
    private:
        TuLing tl;
        aip::Speech *client;

        string appid = "21474279";
        string apikey = "vsIVduroBspw1zrUcp3GzZiR";
        string secretkey = "NCdooNRZENogQ2hxeQnnosx2Q2uuEMXp";

        unordered_map<string, string> cmd_set;
    public:
        Jarvis():client(nullptr){
        }
        void LoadCommandEtc(){
            string name = CMD_ETC;
            ifstream in(name);
            if(!in.is_open()){
                cout << "Load command etc error" << endl;
                exit(1);
            }
            char line[SIZE];
            string sep = ": ";
            while(in.getline(line, sizeof(line))){
                string str = line;
                size_t pos = str.find(sep);
                if(pos == string::npos){
                    cout << "command ect format error" << endl;
                    break;
                }
                string key = str.substr(0, pos);
                string value = str.substr(pos+sep.size());
                cmd_set.insert({key, value});
            }
            in.close();
        }
        void Init(){
            client = new aip::Speech(appid, apikey, secretkey);
            cout << "load command etc ... ing" << endl;
            LoadCommandEtc();
            cout << "load command etc ... done" << endl;
        }
        bool Exec(string cmd, bool is_print){
            FILE *fp = popen(cmd.c_str(), "r");
            if(nullptr == fp){
                cout << "popen error!" << endl;
                return false;
            }

            string result;
            if(is_print){
                char c;
                size_t s = 0;
                while((s = fread(&c, 1, 1, fp)) > 0){
                    result.push_back(c);
                }
                cout << result << endl;
            }

            pclose(fp);
            return true;
        }
        string RecognizePickup(Json::Value &root)
        {
            int err_no = root["err_no"].asInt();
            if(err_no != 0){
                cout << root["err_msg"] << " : " << err_no << endl;
                return "unknown";
            }

            return root["result"][0].asString();
        }
        string ASR(aip::Speech *client)
        {
            string asr_file = VOICE_PATH;
            asr_file += "/";
            asr_file += SPEECH_ASR;

            map<string, string> options;
            string file_content;
            aip::get_file_content(asr_file.c_str(), &file_content);

            Json::Value root = client->recognize(file_content, "wav", 16000, options);
            
            return RecognizePickup(root);
        }

        void Run(){
            string cmd = "arecord -t wav -c 1 -r 16000 -d 5 -f S16_LE ";
            cmd += VOICE_PATH;
            cmd += "/";
            cmd += SPEECH_ASR;
            cmd += ">/dev/null 2>&1";
            for(;;){
                cout << "讲话中...";
                fflush(stdout);
                if(Exec(cmd, false)){
                    cout << endl;
                    cout << "识别中...";
                    fflush(stdout);
                    string message = ASR(client);
                    if(message == "退出。"){
                        cout << "好的" << endl;
                        break;
                    }
                    cout << endl;
                    cout << "我# " << message << endl;
                    string echo = tl.Chat(message);
                    cout << "贾维斯# " << echo << endl;
                }
            }
        }
};

#endif








