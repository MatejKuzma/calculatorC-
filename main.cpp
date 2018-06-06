/*Matej Kuzma jednoducha kalkukacka pre linux
kompilovane cez:

g++ main.cpp -Wall -o calc
*/

#include <iostream>
#include <string>
#include <cctype>
#include <algorithm>
#include <regex>

using namespace std;

// kody errorov pre throw
#define FORMAT_ERR 1
#define ENCAPSULATION_ERR 2
#define ZERO_DIVISION 3

// enum pre kontrolu citania vstupu a typy elementov
enum elementType {subsection, sign, number, begin};

// enum pre typy sektorov
enum sectorType {normal, sqroot, powerof};

typedef string::const_iterator iteratorStr; 

/* datove struktury, ktore vyuziva vypocet a na ktorych je postavena cela 
myslienka rekurzivneho ciastkoveho vypoctu*/
typedef struct section SECTION;
typedef struct element ELEMENT; 

// element - najmensia jednotka vyrazu, obsahuje bud operaciu alebo cislo alebo subsekciu
typedef struct element
{
   elementType type;
   string value;
    ELEMENT * nextElement = NULL;
    SECTION * subsection = NULL; 
}ELEMENT;

/* sekcia, drzitel sekvencie elementov, zaciatok a koniec reprezentuju zatvorky, 
   sekcie sa retazia na seba do roznych hlbok, najnizsia hlbka ma najvacsiu prioritu z hladiskaporadia vypoctu*/
typedef struct section
{
   ELEMENT * begin;
   ELEMENT secElement;
}SECTION;

// globalne premenne pre program
const regex numRGX("^[0-9]+(\\.[0-9]+)?$");
const regex opRGX("^(((\\+|\\-)(\\+|\\-)*)|\\*|\\/)$");

// zmen splet znakov +- na finalnu operaciu
string trimOperation(const string & inputStr)
{
    iteratorStr position = inputStr.begin();

    //ak nieje operacia + alebo - nemen nic, vrat sa
    if(*position == '*' || *position == '/')
        return inputStr;
        
    string tmp = inputStr;
    int actOperation = 1;

    // iteracia cez cely retazec
    while(position != inputStr.end()){
        if(*position == '-')
            actOperation*=-1;
        position++;
    } 

    if(actOperation == -1)
        tmp = '-';
    else
        tmp = '+';
    return tmp;
}

// funkcia na najdenie znaku v retazci ohranicenom jeho iteratormi
bool findCharInSet(char c, const string & set)
{
    return (find(set.begin(),set.end(),c) != set.end());
}

// vrat true ak znak je whitespace, inak false
bool isWhitespace(const char & c)
{ 
    if(isspace(c) == 0)
        return false;
    else
        return true;
}

// pomocna funkcia pre kopirovanie substringu pomocou iteratorov
string strSubset(iteratorStr begin, iteratorStr end)
{
    string str = "";
    while(begin!=end){
        str += *begin;
        begin++;
    }
    return str;
}

// pomocna funkcia pre funkciu processInput, iteracia v spajanom zozname
void chainElement(ELEMENT * & inElement, ELEMENT *& backPtr, elementType typeElement, const string & str)
{
    string tmp = str;
    inElement->type = typeElement;
    if(typeElement == sign)
        tmp = trimOperation(str);

    inElement->value = tmp;
    backPtr = inElement;
    inElement->nextElement = new ELEMENT;
    inElement = inElement->nextElement;
}

// funkcia pre kontrolu spravnosti vstupu a odstranenie whitespace charov a vlozenie do datovych struktur na rekurzivny vypocet
SECTION processInput(const string & inputStr, SECTION & actSection, iteratorStr & stringPos, int & bracketNum){   
    // premenna pre osetrenie nespravnej sekvenecie typov elementov ako operacia-operacia alebo cislo-cislo
    bool elementCheck = false;
    // ak sa na zaciatku nachadza operacia scitania a odcitania, zmen logiku sekvencie citania elementov
    while(isWhitespace(*stringPos)){
             if(stringPos++==inputStr.end()) break;
    }
    if(*stringPos == '-' || *stringPos == '+')
        elementCheck = true;

    // datove struktury do kt. sa budu data z spracovaneho retazca naplnat
   // actSection.secElement = *actSection.begin;
    ELEMENT * backPtr = NULL;
    actSection.begin = &actSection.secElement;
    ELEMENT * actElement = actSection.begin;

    // pomocny iterator
    iteratorStr tmpPos;
    // iteruj cez cely retazec
    while(stringPos != inputStr.end()){
          // preskoc whitespace charaktery
          while(isWhitespace(*stringPos))
             if(stringPos!=inputStr.end()) 
                stringPos++;   
             else
                break;  
       
        if(*stringPos == '('){
            // kontroluj pomer medzi zatvorkami a vnor sa v ramci sekcie
            bracketNum++;
            if(elementCheck)
                throw ENCAPSULATION_ERR;

            // ak je spravny format a uzatvorkovanie tak sa vnor 
             actElement->type = subsection;
             actElement->subsection = new SECTION(); // pre vnorenie je treba vytvorit sekciu koli rekurzii
            *actElement->subsection = processInput(inputStr,*actElement->subsection, ++stringPos, bracketNum);
             actElement->nextElement = new ELEMENT(); // priprav dalsi element a zretaz
             backPtr = actElement;
             actElement = actElement->nextElement;
   
            elementCheck = true;
            continue;
        }
        else if(*stringPos == ')'){
            if(--bracketNum < 0 || !elementCheck)
                throw ENCAPSULATION_ERR;
            ++stringPos;
             
             if(backPtr != NULL)
                backPtr->nextElement = NULL;
            return actSection;  
        }
        else{
            tmpPos = stringPos;
            // kopirovanie znakov z mnoziny cisla
            while(findCharInSet(*tmpPos, "0123456789."))
                    tmpPos++;
            
             // REGEX test na cislo
            if(regex_match(stringPos,tmpPos, numRGX)){
                if(elementCheck) // ak bol predosly element cislo alebo subsekcia a znova citame cislo -> chyba
                    throw FORMAT_ERR;
            
                chainElement(actElement, backPtr, number, strSubset(stringPos, tmpPos));
                stringPos = tmpPos;
                elementCheck = true; 
                continue;
            } 
            // kopirovanie znakov z mnoziny operacie
            while(findCharInSet(*tmpPos, "+-*/") && tmpPos != inputStr.end())
                    tmpPos++;
           
            // REGEX test na operaciu
           if(regex_match(stringPos,tmpPos, opRGX)){
                if(!elementCheck)  // Ak po operacii nasleduje dalsia operacia -> chyba
                     throw FORMAT_ERR;
                // zretaz novy element
                chainElement(actElement, backPtr, sign, strSubset(stringPos, tmpPos));
                stringPos = tmpPos;
                elementCheck = false;
                continue;
           }
           throw FORMAT_ERR;
        }
    } 
    // ak nesedi pocet zatvoriek alebo na konci sa ocitla operacia
    if(bracketNum != 0 || !elementCheck) 
        throw 4;

    if(backPtr != NULL)
        backPtr->nextElement = NULL;
    return actSection;    
}

float countInput(SECTION & actSection);

// pomocna funkcia pre countInput pre sprehladnenie funkcie
float count(  ELEMENT * inElement)
{
     if (inElement->type == subsection)
          return countInput(*inElement->subsection);
     else
          return atof(inElement->value.c_str());
}

string floatToStr(float in)
{
    std::ostringstream ss;
    ss << in;
    string tmpStr(ss.str());
    return tmpStr;
}

// vypocet z datovej struktury
float countInput(SECTION & actSection)
{
    ELEMENT * actElement = actSection.begin;
    ELEMENT * signElement = NULL;
    ELEMENT * secondElement;
    float output = 0.0f, leftValue, rightValue;

    // ak je prvy element operacia, nasobi sa to s dalsim elementom cisla a retazi sa
    if (actElement->type == sign){
        // najdi element, ktory prislucha danemu znamienku, mal by byt hned zanim
        secondElement = actElement->nextElement;
        output = count(secondElement);
        // ak je prvy znak minus unvertuj hodnotu nasledujuceho cisla
        if(actElement->value.at(0) == '-')
            output *= -1;
        // posun ukazovatel
        actElement = actElement->nextElement; 
        actElement->type = number;
        actElement->value = floatToStr(output);
        actSection.begin = actElement;
    }

    // ak sa v subsekcii nachadza iba jeden element vrat hodnotu
    if (actElement->nextElement == NULL){
        output = count(actElement);
        return output;
    }

    // hlavna slucka, iteruj cez spajany, hladaj naprv operacie * a /
   while(actElement->nextElement != NULL){
//cout << "SKACEM PO */ " << actElement << " ( " << actElement->value << " )" << endl;
        signElement = actElement->nextElement;
        // ak sa operacia nieje 
        if(signElement->value.at(0) != '*' && signElement->value.at(0) != '/'){
            actElement = signElement->nextElement;
            continue;
        }

        // vypocitanie lavej hodnoty
        leftValue = count(actElement);
        // pocitanie pravej hodnoty
        secondElement = signElement->nextElement;
        rightValue = count(secondElement);

        // urob operaciu nad dvoma hodnotami
        if(signElement->value.at(0) == '*')
        {
//cout << leftValue << " * " << rightValue << endl;
             leftValue = leftValue * rightValue;
        }

        else{
            if(rightValue == 0)
               throw ZERO_DIVISION; // ak by malo nastat delenie nulou ukonci
                    
//cout << leftValue << " / " << rightValue << endl;
            leftValue = leftValue / rightValue;
        }
        // urob nove zretazenie elementov
        actElement->nextElement = secondElement->nextElement;
        actElement->type = number;
        actElement->value = floatToStr(leftValue);
    }

    actElement = actSection.begin;
    while(actElement->nextElement != NULL){
//cout << "SKACEM PO +-" << actElement << " ( " << actElement->value << " )" << endl;
        signElement = actElement->nextElement;
        
         // vypocitanie lavej hodnoty
        leftValue = count(actElement);
        // pocitanie pravej hodnoty
        secondElement = signElement->nextElement;
        rightValue = count(secondElement);

          // urob operaciu nad dvoma hodnotami
        if(signElement->value.at(0) == '+')
        {
//cout << leftValue << " + " << rightValue << endl;
             leftValue = leftValue + rightValue;             
        }
        else
        {
//cout << leftValue << " - " << rightValue << endl;
            leftValue = leftValue - rightValue;
        }
         // urob nove zretazenie elementov
        actElement->nextElement = secondElement->nextElement;
        actElement->type = number;
        actElement->value = floatToStr(leftValue);
    }
    return atof(actElement->value.c_str());
}

// zoskupenie krokov potrebnych na vypocet retazca
void countString(string & inputStr)
{
    /* 1. FAZA: predspracuj vstup -> odstran whitespace, skontroluj spravnost vstupu, 
      roztried vstup do datovych struktur zretazenych segmentov a elementov stylom cislo - operacia - cislo*/
    iteratorStr strStart = inputStr.begin();
    int bracketNum = 0;
    SECTION * baseSection = new SECTION;
    processInput(inputStr,*baseSection, strStart, bracketNum);
        
   // 2. FAZA: vypocitaj vysledok
   cout << "= " << countInput(*baseSection) << endl;
   delete baseSection;
}

int main()
{
    string inputLine;
    // Vstup: Citaj riadok po riadku retazec znakov az po terminaciu
    int lineIndex = 0;
    while(getline(cin, inputLine)){
        ++lineIndex;
        // vypocitaj retazec zo vstupu
        try{
            cout << lineIndex << ": " << inputLine << endl;
            countString(inputLine);
        }
        catch(int err){
            cerr << "Bad input format !" << endl;
        }
        cout << endl;
    }
   return 0;
}
