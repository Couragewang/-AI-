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
#include "log.hpp"
#include "speech.h"
#include "base/http.h"

#define VOICE_PATH "voice"
#define SPEECH_ASR "asr.wav"
#define SPEECH_TTL "ttl.mp3"
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
                LOG(Warning, "jsoncpp parse error");
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
                LOG(Waring, "http request error!");
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

        string record; //
        string play;
        unordered_map<string, string> record_set;
    public:
        Jarvis():client(nullptr){
        }
        void LoadCommandEtc(){
            LOG(Normal, "command etc load begin!");
            string name = CMD_ETC;
            ifstream in(name);
            if(!in.is_open()){
                LOG(Warning, "Load command etc error");
                exit(1);
            }
            char line[SIZE];
            string sep = ": ";
            while(in.getline(line, sizeof(line))){
                string str = line;
                size_t pos = str.find(sep);
                if(pos == string::npos){
                    LOG(Warning,"command ect format error");
                    break;
                }
                string key = str.substr(0, pos);
                string value = str.substr(pos+sep.size());
                key += "。"; //语音识别能够对比成功

                record_set.insert({key, value});
            }
            in.close();
            LOG(Normal, "command etc load success!");
        }
        void Init(){
            client = new aip::Speech(appid, apikey, secretkey);
            LoadCommandEtc();

            record = "arecord -t wav -c 1 -r 16000 -d 5 -f S16_LE ";
            record += VOICE_PATH;
            record += "/";
            record += SPEECH_ASR;
            record += ">/dev/null 2>&1";


            play = "cvlc --play-and-exit ";
            play += VOICE_PATH;
            play += "/";
            play += SPEECH_TTL;
            play += ">/dev/null 2>&1";
        }
        bool Exec(string command, bool is_print){
            FILE *fp = popen(command.c_str(), "r");
            if(nullptr == fp){
                cout << "popen error!" << endl;
                return false;
            }

            if(is_print){
                char c;
                size_t s = 0;
                while((s = fread(&c, 1, 1, fp)) > 0){
                    cout << c;
                }
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
        bool TTL(aip::Speech *client, std::string &str)
        {
            ofstream ofile;
            string ttl = VOICE_PATH;
            ttl += "/";
            ttl += SPEECH_TTL;
            ofile.open(ttl.c_str(), ios::out | ios::binary);

            string file_ret;
            map<string, string> options;

            options["spd"] = "6";
            options["per"] = "4";
            options["vol"] = "8";

            Json::Value result = client->text2audio(str, options, file_ret);
            if(!file_ret.empty()){
                ofile << file_ret;
            }
            else{
                cout << result.toStyledString() << endl;
            }
            ofile.close();
        }
        
        bool IsCommand(std::string &message)
        {
            return record_set.find(message) != record_set.end() ? true : false;
        }

        void Run(){
            for(;;){
                LOG(Normal, "....................................讲话中");
                fflush(stdout);
                if(Exec(record, false)){
                    LOG(Normal, ".....................................识别中");
                    string message = ASR(client);
                    cout << endl;
                    LOG(Normal, message);
                    //if("退出." )
                    if(IsCommand(message)){
                        //是命令
                        LOG(Normal, "Exec a command!");
                        if(record_set[message] == "quit"){
                            break;
                        }
                        Exec(record_set[message], true);
                        continue;
                    }
                    else{
                        //不是命令    
                        LOG(Normal, "run a normal chat!");
                        //cout << "我# " << message << endl;
                        string echo = tl.Chat(message);
                        LOG(Normal, echo);
                        //cout << "贾维斯# " << echo << endl;
                        TTL(client, echo);
                        Exec(play, false);
                        //text to mp3
                    }
                }
            }
        }
};

#endif








