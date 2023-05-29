// Manuel Villalpando
// Erik Cabrera

#include <iostream>
#include <fstream>
#include <regex>
#include <pthread.h>
#include <filesystem>
#include <ctime>

using namespace std;
namespace fs = std::filesystem;

// Prototipos
string lexer(const string &input);
void htmlFile(const string &tokenizedCode, const string &filename);
double secuentialExecution(const string &folderPath);
double parallelExecution(const string &folderPath);
void *threadFunction(void *args);

int main(int argc, char *argv[]) {
    double secuentialTime, parallelTime;
    string folderPath = argv[1];

    // Revisar si existe folder
    if (argc < 2) {
        cout << "Uso: lexer <folder_path>" << endl;
        return 0;
    }

    // Revisar si path existe
    if (!fs::exists(folderPath) || !fs::is_directory(folderPath)) {
        cout << "Folder no valido" << endl;
        return 0;
    }

    secuentialTime = secuentialExecution(folderPath);
    parallelTime = parallelExecution(folderPath);

    cout << "Secuencial: " << secuentialTime << " segundos" << endl;
    cout << "Paralelo: " << parallelTime - secuentialTime << " segundos" << endl;
    cout << "Diferencia: " << secuentialTime / (parallelTime - secuentialTime) << "x" << endl;

    return 0;
}

string lexer(const string &input) {
    string tokenizedCode;

    // Expresiones regulares
    const string keywords = "abstract|as|base|bool|break|byte|case|catch|char|checked|class|const|continue|decimal|default|delegate|double|do|else|enum|event|explicit|extern|false|finally|fixed|float|foreach|for|foreach|goto|if|implicit|int|in|interface|internal|is|lock|long|namespace|new|null|object|operator|out|override|params|private|protected|public|readonly|ref|return|sbyte|sealed|short|sizeof|stackalloc|static|string|struct|switch|this|throw|true|try|typeof|uint|ulong|unchecked|unsafe|ushort|using|virtual|void|volatile|while";
    const string identifiers = "[a-zA-Z_][a-zA-Z0-9_]*";
    const string operators = "\\+|-|\\|/|%|\\^|&|\\||~|!|=|<|>|\\?|:|;|,|\\.|\\+\\+|--|&&|\\|\\||==|!=|<=|>=|\\+=|-=|\\=|/=|%\\=|\\^=|&\\=|\\|=|<<=|>>=|=>|\\?\\?";
    const string literals = "[0-9]+(\\.[0-9]+)?|\".*\"|'.*'";
    const string comments = "//.*|/\\*.*\\*/";
    const string separators = "[\\(\\)\\{\\}\\[\\];,.]";
    const string lineBreak = "\n";
    const string whiteSpace = "\\s+";
    const regex allTokens(keywords + "|" + identifiers + "|" + operators + "|" + literals + "|" + comments + "|" + separators + "|" + lineBreak + "|" + whiteSpace);

    // Tokens
    auto current = sregex_iterator(input.begin(), input.end(), allTokens);
    const auto end = sregex_iterator();

    while (current != end) {
        const string token = (*current).str();

        if (token == "\n") {
            tokenizedCode += "<br>";
        }
        else {
            string type;

            if (regex_match(token, regex(lineBreak))) {
                tokenizedCode += "</pre><pre>";
            }
            else if (regex_match(token, regex(whiteSpace))) {
                tokenizedCode += token;
            }
            else if (regex_match(token, regex(operators))) {
                type = "operator";
            }
            else if (regex_match(token, regex(comments))) {
                type = "comment";
            }
            else if (regex_match(token, regex(keywords))) {
                type = "keyword";
            }
            else if (regex_match(token, regex(literals))) {
                type = "literal";
            }
            else if (regex_match(token, regex(separators))) {
                type = "separator";
            }
            else if (regex_match(token, regex(identifiers))) {
                type = "identifier";
            }
            else {
                type = "error";
            }

            tokenizedCode += "<span class=\"" + type + "\">" + token + "</span>";
        }

        ++current;
    }
    return tokenizedCode;
}

// HTML
void htmlFile(const string &tokenizedCode, const string &filename) {
    string outputFilename = fs::path(filename).stem().string() + ".html";
    string outputFilePath = "./output/" + outputFilename;

    if (!fs::exists("./output/")) {
        fs::create_directory("./output/");
    }

    // Estilos
    string html = R"(
        <!DOCTYPE html>
        <html>
        <head>
        <style>
            body {
                background-color: #25292E;
                color: #f8f8f2;
                font-family: Consolas, monospace;
                font-size: 18px;
                margin: 40px;
            }
            pre {
                margin: 0px;
            }
            span {
                display: inline-block;
            }
            span.keyword {
                color: #A020f0;
            }
            span.identifier {
                color: #cccccc;
            }
            span.separator {
                color: #F7F486;
            }
            span.operator {
                color: #D3747C;
            }
            span.literal {
                color: #bf00ff;
            }
            span.comment {
                color: #75715e;
            }
        </style>
        </head>
        <body>
        <pre>
    )";

    html += tokenizedCode;

    html += R"(
        </pre>
        </body>
        </html>
    )";

    ofstream outfile(outputFilePath, ios::app);
    if (outfile.is_open()) {
        outfile << html;
        outfile.close();
    }
    else {
        cout << "Error al abrir el archivo" << endl;
    }
}

// Secuencial
double secuentialExecution(const string &folderPath) {
    cout << "Secuencial..." << endl;

    clock_t start = clock();

    for (const auto &entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".cs") {
            string filePath = entry.path().string();
            ifstream file(filePath);
            if (!file.is_open()) {
                cout << "Error al abrir el archivo"<< endl;
                continue;
            }

            string input((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
            string tokenizedCode = lexer(input);
            htmlFile(tokenizedCode, filePath);
        }
    }
    
    clock_t end = clock();
    double secExecTime = double(end - start) / CLOCKS_PER_SEC;

    return secExecTime;
}

struct ThreadArgs {
    string filename;
    string folderPath;
};

// Hilos
void *threadFunction(void *args) {
    ThreadArgs *threadArgs = (ThreadArgs *)args;
    string filename = threadArgs->filename;
    string folderPath = threadArgs->folderPath;

    ifstream file(folderPath + "/" + filename);
    string input((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    string tokenizedCode = lexer(input);

    htmlFile(tokenizedCode, filename);

    pthread_exit(NULL);

    return NULL;
}

// Paralelo
double parallelExecution(const string &folderPath) {
    cout << "Paralelo..." << endl;

    clock_t start = clock();

    pthread_t threads[110];
    int threadIndex = 0;

    for (const auto &entry : fs::directory_iterator(folderPath)) {
        string filename = entry.path().filename().string();

        ThreadArgs *threadArgs = new ThreadArgs;
        threadArgs->filename = filename;
        threadArgs->folderPath = folderPath;

        pthread_create(&threads[threadIndex], NULL, threadFunction, (void *)threadArgs);

        threadIndex++;
    }

    for (int i = 0; i < threadIndex; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_t end = clock();
    double parExecTime = double(end - start) / double(CLOCKS_PER_SEC);

    return parExecTime;
}