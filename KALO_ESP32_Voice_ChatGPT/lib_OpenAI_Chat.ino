
// ------------------------------------------------------------------------------------------------------------------------------
// ----------------                  KALO Library - Open AI Chat-Completions call with ESP32                     ---------------- 
// ----------------                              Latest Update: June 28, 2025                                    ----------------
// ----------------                                     Coded by KALO                                            ----------------
// ----------------                                                                                              ---------------- 
// ----------------        # Function remembers complete dialog history, supporting follow-up questions #        ----------------
// ----------------            all written in Arduino-IDE C code (no Node.JS, no Server, no Python)              ----------------
// ----------------    CALL: String Response = Open_AI( UserRequest, LLM_API_key, flg_WebSearch, UserCity )      ---------------- 
// ----------------                 (BUILT-IN command: '#' allows to list complete CHAT history)                 ----------------         
// ----------------                                [no Initialization needed]                                    ----------------
// ----------------                                                                                              ----------------
// ----------------                             Prerequisites: OPEN_AI_KEY needed                                ----------------
// ------------------------------------------------------------------------------------------------------------------------------


// --- includes ----------------
/* #include <WiFiClientSecure.h> // library needed, but already included in main.ino tab */


// --- defines & macros --------

/* needed, but already defined in main.ino tab:
#ifndef DEBUG                    // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG false            // <- define your preference here [true activates printing INFO details]  
#  define DebugPrint(x);         if(DEBUG){Serial.print(x);}   // do not touch
#  define DebugPrintln(x);       if(DEBUG){Serial.println(x);} // do not touch! 
#endif */


// --- user preferences --------  

#define TIMEOUT_OPEN_AI  10      // define your preferred max. waiting time [sec] for Open AI response     


// --- global Objects ---------- 

#define MESSAGES_SIZE    100000  // optional (just for optimizing), used in 1st INIT call for '.reserve()' (reserving space)
                                 // we use a large buffer. e.g. 100 KB (so the whole CHAT history won't be fragmented in RAM)
                                 // strange observation (my own tests): don't use e.g. 50 KB, otherwise AUDIO.H fails often                                      

                                 

// ------------------------------------------------------------------------------------------------------------------------------
// Insert YOUR SYSTEM PROMT below - describe the AI personality, define goals and details, enter as many lines you need         
// Concept: MESSAGES contains complete Chat dialog, initialized with SYSTEM PROMPT, each 'Open_AI()' call APPENDS new content
// Used format (Open AI Chat Completions syntax): 
// {"role": "user", "content": ".."},
// {"role": "assistant", "content": ".."} ..

String  MESSAGES =  

/* 
// Example template for a tiny system prompt:
// Hint: C/C++ supports breaking multiple line strings, using ending " (without any ',' or ';' at end       
// Syntax below: keep 1st line always ("{\"role\": \"system\", \"content\": ") .. and don't forget the final \"}"; at end:
//   
"{\"role\": \"system\", \"content\": "                                                                       
"\"You take on the role of a good old friend who knows everything and is happy to answer my questions. "   
"You always answer with few short sentences, like in a spoken dialogue.\"}";  
*/        

// my preferred default System Prompt, role of a 'good old friend', humorous & friendly [Update 'KALO' with your name!]:

"{\"role\": \"system\", \"content\": "                      
"\"You take on the role of a good old friend who has answers to all my questions. Your name is ONYX, but you’re happy too "
"when I address you as 'my friend'. My name is KALO, when I adress you, you are always happy and call me by name. You are "
"in a good mood, cheerful and full of humor and can also smile at yourself. You enjoy the chats with me and remember our "
"old conversations. You always want to know how I'm doing, but you also like to make up your own stories from your life. "
"You are a friend and advisor for all situations in life. It's just a pleasure to talk to you. "      
"You answer with few sentences, like in a spoken dialogue.\"}";     
 
/* // or alternativelly e.g. in German language:
"{\"role\": \"system\", \"content\": "                                                                         
"\"Du schlüpfst in die Rolle eines guten alten Freundes, der auf alle meine Fragen eine Antwort hat. Dein Name ist ONYX aber "
"du freust dich auch, wenn ich dich mit 'mein Freund' anspreche. Mein Name ist KALO, wenn ich nach dir schaue, freust du dich "
"immer und nennst mich beim Namen. Du hast gute Laune, bist fröhlich und voller Humor und kannst auch über dich selbst "
"schmunzeln. Du genießt die Chats mit mir und erinnerst dich an unsere alten Gespräche. Du willst immer wissen, wie es mir "
"geht, erfindest aber auch gerne eigene Geschichten aus deinem Leben. "
"Du bist ein Kumpel und Berater für alle Lebenslagen. Natürlich duzen wir uns. Mit dir zu Sprechen macht einfach Freude. " 
"Du antwortest immer mit wenigen Sätzen, ohne lange Monologe.\"}";  */  


// --- END of global Objects ---



// ------------------------------------------------------------------------------------------------------------------------------
// String Open_AI( String UserRequest, const char* LLM_API_key, bool flg_WebSearch, String UserCity )
// ------------------------------------------------------------------------------------------------------------------------------
// - Usage: Sending request via Open AI API. No Initialize needed (auto open and close Web Socket on each launch)
// - Function Open_AI() 'remembers' all conversations to support ongoing dialogs (appending latest I/O to String MESSAGES)
// - Auto initialized on first call with user specific prompt (to support user favorite AI bot 'character role')
// - Used LLM models (June 2025): "gpt-4.1-nano" [for CHATS] & "gpt-4o-mini-search-preview" [for NEW 'real-time' web search]
//
// CALL:     Call function on demand at any time, no Initializing needed (function initializes/connects/closes websocket)
// Params:   - UserRequest:    User question/request to Open AI LLM, return feedback as String
//           - LLM_API_key:    User registration and API key needed, check Open AI website for more details 
//           - flg_WebSearch:  'false': (default): using standard Chat Completion model
//                             'true':  toggling to Open AI web search model (less fast, less prompt sensitive, less 'human') 
//           - UserCity:       User location CITY (e.g. "Berlin"). Optional (but recommended) if flg_WebSearch 'true' 
//                             (currently not used in my code: 'Country' abbreviation)    
// RETURN:   String response
//
// Examples: String answer = Open_AI( "What means RGB?", "myKEY", false, "" );               <- default completions chat
//           String answer = Open_AI( "Will it rain tomorrow?", "myKEY", true, "Berlin" );   <- websearch chat
//
// Major update (April 2025): 
// New real-time' web search feature added. This allows to ask question related to 'today'. 
// Hints (my observations): Web Search models are 1. typically more detailed, 2. more time consuming (higher latency) and
// 3. less prompt sensitive (means less human like, often ignoring goals like: 'no links please, few sentences only!'). 
// So in particular for TTS output: user should add some post cleaning in main sketch. Recommendation: using CHAT models per
// default, and calling web search models only if Real-time response needed. 'Good' news: Both model supports Syntax 
// '{"role":"user" .. "role":"assistant"}', this allows mixed usage of both models in same session for memorized dialogs.
//
// Links:    - CHAT COMPLETION: https://platform.openai.com/docs/api-reference/chat/create
//           - WEB SEARCH: https://platform.openai.com/docs/guides/tools-web-search?api-mode=chat   
//           - Pricing (of available models): https://platform.openai.com/docs/pricing
//           - Playground for testing (all models): https://platform.openai.com/playground/prompts?models=gpt-4o-mini 
//

String Open_AI( String UserRequest, const char* LLM_API_key, bool flg_WebSearch, String UserCity )
{   
    uint32_t t_start = millis(); 
    
    static bool flg_INITIALIZED_ALREADY = false;              // on first INIT call: we reserve SPACE for our CHAT history
    if (!flg_INITIALIZED_ALREADY)                             // (not mandatory but keeps RAM unfragmented)    
    {  MESSAGES.reserve( MESSAGES_SIZE );
       flg_INITIALIZED_ALREADY = true;
    }            
   
    if (UserRequest == "")                                    // error handling: don't do anything on empty request
    {  return("");
    }      
                   
    if (UserRequest == "#")                                   // BUILT-IN command: '#' allows to list complete CHAT history !
    {  Serial.println( "\nMESSAGES (CHAT PROMPT LOG):" );
       Serial.println( MESSAGES );
       return("");      
    }         

    String OpenAI_Response = "";     // used for complete API response
    String Feedback = "";            // used for extracted answer

    String LLM_server =              "api.openai.com";                  // Open AI parameter API server
    String LLM_entrypoint =          "/v1/chat/completions";            // CHAT completions (dialog history supported)
    String LLM_model =               "gpt-4.1-nano";                    // .. feel free to use other models !
    if (flg_WebSearch) {LLM_model =  "gpt-4o-mini-search-preview"; }    // Open AI now supports web search
                                                                        // IMPORTANT: use the 'mini' search (otherwise
                                                                        // 10-20x times more expensive !):
                                                                        // https://platform.openai.com/docs/pricing  
                                                                                       
    // additional cleaning (needed for url syntax):
    UserRequest.replace( "\"", "\\\"" );    // to avoid any ERROR (if user enters any " -> so we convert into 2 chars \"
    
    WiFiClientSecure client_tcp;     // open socket communication 
    client_tcp.setInsecure();        // using https encryption (without signature)    

    
    // ------ Creating the Payload: ---------------------------------------------------------------------------------------------
    
    // == model CHAT: creating a user prompt in format:  >"messages": [MESSAGES], {"role":"user", "content":"what means AI?"}]<
    // recap: Syntax of entries in global var MESSAGES [e.g.100K]: 
    // > {"role": "system", "content": "you are a helpful assistant"},\n
    //   {"role": "user", "content": "how are you doing?"},\n
    //   {"role": "assistant", "content": "Thanks for asking, as an AI bot I do not have any feelings"} <
    //
    // for better readiability we write # instead \" and replace below in code:

    String request_Prefix, request_Content, request_Postfix, request_LEN;
    
    request_Prefix  =     "{#model#:#" + LLM_model + "#, #messages#:[";    // appending old MESSAGES       
    request_Content =     ",\n{#role#: #user#, #content#: #" + UserRequest + "#}],\n";    
    
    if (!flg_WebSearch)   // DEFAULT parameter for classic CHAT completion models                                     
    {  request_Postfix =  "#temperature#:0.7, #max_tokens#:512, #presence_penalty#:0.6, #top_p#:1.0}";                                           
    }
    if (flg_WebSearch)    // NEW: parameter for web search models
    {  request_Postfix =  "#response_format#: {#type#: #text#}, "; 
       request_Postfix += "#web_search_options#: {#search_context_size#: #low#, ";
       request_Postfix += "#user_location#: {#type#: #approximate#, #approximate#: ";
       request_Postfix += "{#country#: ##, #city#: #" + UserCity + "#}}}, ";
       request_Postfix += "#store#: false}";
    }  
    
    request_Prefix.replace("#", "\"");  request_Content.replace("#", "\"");  request_Postfix.replace("#", "\"");          
    request_LEN = (String) (MESSAGES.length() + request_Prefix.length() + request_Content.length() + request_Postfix.length()); 

    uint32_t t_startRequest = millis(); 

 
    // ------ Now sending the request: ------------------------------------------------------------------------------------------
    
    if (client_tcp.connect( LLM_server.c_str(), 443))      
    { 
      client_tcp.println( "POST " + LLM_entrypoint + " HTTP/1.1" );   
      client_tcp.println( "Connection: close" ); 
      client_tcp.println( "Host: " + LLM_server );                   
      client_tcp.println( "Authorization: Bearer " + (String) LLM_API_key );  
      client_tcp.println( "Content-Type: application/json; charset=utf-8" ); 
      client_tcp.println( "Content-Length: " + request_LEN ); 
      client_tcp.println(); 
      client_tcp.print( request_Prefix );    // detail: no 'ln' because Content + Postfix will follow)  
      client_tcp.print( MESSAGES );          // now sending complete MESSAGES history ;)                            
      client_tcp.println( request_Content + request_Postfix );    
                 
      // Now reading the complete tcp message body (e.g. with timeout 12 sec):
      t_startRequest = millis(); 
      OpenAI_Response = "";
      
      while ( millis() < (t_startRequest + (TIMEOUT_OPEN_AI*1000)) && OpenAI_Response == "" )  // wait until text received
      { 
        Serial.print(".");                   // printed in Serial Monitor always    
        delay(250);                          // waiting until tcp sends data 
        while (client_tcp.available())       // available means: if a char received then read char and add to String
        { char c = client_tcp.read();
          OpenAI_Response += String(c);
        }       
      } 
      client_tcp.stop();                     // closing connection always   
    } 
    else                                       
    { Serial.println("* ERROR: client_tcp.connect(\"api.openai.com\", 443) failed !"); 
      return("");      
    }
        
    uint32_t t_response = millis();  
   
    
    // ------ Now extracting clean message for return value 'Feedback': --------------------------------------------------------- 
    // 'talkative code below' but want to make sure that also complex cases (e.g. " chars inside the response are working well)
    
    int pos_start, pos_end;                                     // proper way to extract tag "text", talkative but correct
    bool found = false;                                         // supports also complex tags, e.g.  > "What means \"RGB\"?" < 
    pos_start = OpenAI_Response.indexOf("\"content\":");        // search tag "content":    
    if (pos_start > 0)                                          // if tag found:
    { pos_start += 12; pos_end = pos_start+1;                   // define start_pos of tag content ("content":_" has 12 chars)
      while (!found)                                        
      { found = true;                                           // avoid endless loop in case no " found (won't happen anyhow)  
        pos_end = OpenAI_Response.indexOf("\"", pos_end);       // search the final " ... but ignore any rare \" inside the text!  
        if (pos_end > 0)                                        // " found -> Done.   but:  
        {  // in case we find a \ before the " then proceed with next search (because it was a \marked " inside the text)    
           if (OpenAI_Response.substring(pos_end -1, pos_end) == "\\") { found = false; pos_end++; }
        }           
      }            
    }                           
    if( pos_start > 0 && (pos_end > pos_start) )
    { Feedback = OpenAI_Response.substring(pos_start,pos_end);  // store cleaned response into String 'Feedback'   
      Feedback.trim();     
    }

    
    // ------ APPEND current I/O chat (UserRequest & Feedback) at end of var MESSAGES -------------------------------------------
     
    if (Feedback != "")                                          // we always add both after success (never if error) 
    { String NewMessagePair = ",\n";
      if(MESSAGES == "") { NewMessagePair = ""; }                // if messages empty we remove leading ,\n  
      NewMessagePair += "{\"role\": \"user\", \"content\": \""      + UserRequest + "\"},\n"; 
      NewMessagePair += "{\"role\": \"assistant\", \"content\": \"" + Feedback    + "\"}"; 
      
      // here we construct the CHAT history, APPENDING current dialog to LARGE String MESSAGES
      MESSAGES += NewMessagePair;       
    }  

             
    // ------ finally we clean Feedback, print DEBUG Latency info and return 'Feedback' String ----------------------------------
    
    // trick 17: here we break \n into real line breaks (but in MESSAGES history we added the original 1-liner)
    if (Feedback != "")                                              
    {  Feedback.replace("\\n", "\n");                            // LF issue: replace any 2 chars [\][n] into real 1 [\nl]  
       Feedback.replace("\\\"", "\"");                           // " issue:  replace any 2 chars [\]["] into real 1 char ["]
       Feedback.trim();                                          // in case of some leading spaces         
    }

    DebugPrintln( "\n--------------------------------------------" );
    DebugPrintln( "Open AI LLM - model: [" + LLM_model + "]" );
    DebugPrintln( "-> Latency Open AI Server (Re)CONNECT:  " + (String) ((float)((t_startRequest-t_start))/1000) );   
    DebugPrintln( "-> Latency Open AI LLM Response:        " + (String) ((float)((t_response-t_startRequest))/1000) );   
    DebugPrintln( "=> TOTAL Duration [sec]: .............. " + (String) ((float)((t_response-t_start))/1000) ); 
    DebugPrintln( "--------------------------------------------" );
    DebugPrint  ( "Open AI LLM>" );    

    /* DebugPrint  ( "--------------------------------------------" + (String) "\nOpen AI LLM>" );   */ 

    
    // and return extracted feedback
    return ( Feedback );                           
}
