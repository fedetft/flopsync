
//Tx board
const int txRst=2;
const int txBoot=3;
//Rx board 1
const int rx1Rst=4;
const int rx1Boot=5;
//Rx board 2
const int rx2Rst=6;
const int rx2Boot=7;
//Event I/O
const int event=8;

class Device
{
public:
    Device(int rst, int boot) : rst(rst), boot(boot)
    {
        pinMode(boot,OUTPUT);
        digitalWrite(boot,LOW); 
        pinMode(rst,INPUT); //Let the pullup do the job
    }
    
    void enterFirmwareUpgrade()
    {
        pinMode(rst,OUTPUT);
        digitalWrite(rst,LOW);
        digitalWrite(boot,HIGH);
        delay(500);
        pinMode(rst,INPUT); //Let the pullup do the job
    }
    
    void leaveFirmwareUpgrade()
    {
        pinMode(rst,OUTPUT);
        digitalWrite(rst,LOW);
        digitalWrite(boot,LOW);
        delay(500);
        pinMode(rst,INPUT); //Let the pullup do the job
    }
    
    void keepReset()
    {
        pinMode(rst,OUTPUT);
        digitalWrite(rst,LOW);
        digitalWrite(boot,LOW);
    }
    
private:
    int rst, boot;
};

void setup() {}

void loop()
{
    Serial.begin(19200);
    pinMode(event,OUTPUT);
    Device tx(txRst,txBoot);
    Device rx1(rx1Rst,rx1Boot);
    Device rx2(rx2Rst,rx2Boot);
    Device *devices[3]={&tx,&rx1,&rx2};
    char line[32];
    for(;;)
    {
        Serial.println("'numdev op'|'e' numdev={0=tx,1=rx1,2=rx2},op={e=enter,l=leave,k=keep reset},e=fire event");
        for(int i=0;i<sizeof(line);)
        {
            int c=Serial.read();
            if(c==-1) continue;
            line[i]=c;
            if(i==0 && line[0]=='\n') continue;
            if(line[i]=='\n' || line[i]=='\r') { line[i]='\0'; break; }
            i++;
        }
        if(strcmp(line,"e")==0)
        {
            Serial.println("event");
            digitalWrite(event,HIGH);
            delayMicroseconds(100);
            digitalWrite(event,LOW);
        } else {
            int a=-1;
            char c='x';
            if(sscanf(line,"%d %c",&a,&c) && a>=0 && a<=2)
            {
                bool error=false;
                switch(c)
                {
                    case 'e':
                        devices[a]->enterFirmwareUpgrade();
                        break;
                    case 'l':
                        devices[a]->leaveFirmwareUpgrade();
                        break;
                    case 'k':
                        devices[a]->keepReset();
                        break;
                    default:
                        error=true;
                }
                if(error) Serial.println("error");
                else Serial.print(a); Serial.print(" "); Serial.println(c);
            } else Serial.println("error");
        }
    }
}
