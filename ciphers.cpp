
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "include/caesar_dec.h"
#include "include/caesar_enc.h"
#include "include/subst_dec.h"
#include "include/subst_enc.h"
#include "utils.h"

using namespace std;

// Initialize random number generator in .cpp file for ODR reasons
std::mt19937 Random::rng;

const string ALPHABET = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

// Function declarations go at the top of the file so we can call them
// anywhere in our program, such as in main or in other functions.
// Most other function declarations are in the included header
// files.

// When you add a new helper function, make sure to declare it up here!

/**
 * Print instructions for using the program.
 */
void printMenu();
// --- Substitution decrypt function declarations (needed before main) ---
double scoreString(const QuadgramScorer& scorer, const string& s);
void computeEnglishnessCommand(const QuadgramScorer& scorer);
vector<char> hillClimb(const QuadgramScorer& scorer, const string& ciphertext);
void decryptSubstCipherCommand(const QuadgramScorer& scorer);
void decryptSubstCipherFileCommand(const QuadgramScorer& scorer);

int main() {
  Random::seed(time(NULL));
  string command;

  ifstream file("english_quadgrams.txt");
  vector<string> quadgrams;
  vector<int> counts;
  string line;
  while (getline(file, line)) {
    int comma = line.find(',');
    string quad = line.substr(0, comma);
    int count = stoi(line.substr(comma + 1));
    quadgrams.push_back(quad);
    counts.push_back(count);
  }

  QuadgramScorer scorer(quadgrams, counts);

  cout << "Welcome to Ciphers!" << endl;
  cout << "-------------------" << endl;
  cout << endl;

  do {
    printMenu();

    cout << endl << "Enter a command (case does not matter): ";

    getline(cin, command);

    if (command == "D" || command == "d") {
      ifstream dictFile("dictionary.txt");
      vector<string> dict;
      string w;

      while (getline(dictFile, w)) {
        w = clean(w);
        if (!w.empty()) dict.push_back(w);
      }

      caesarDecryptCommand(dict);
    }

    if (command == "C" || command == "c") {
      caesarEncryptCommand();
    }
    cout << endl;

    if (command == "S" || command == "s") {
      decryptSubstCipherCommand(scorer);
    }

    if (command == "E" || command == "e") {
      computeEnglishnessCommand(scorer);
    }

    if (command == "A" || command == "a") {
      applyRandSubstCipherCommand();
    }

    if (command == "F" || command == "f") {
      decryptSubstCipherFileCommand(scorer);
    }

    if (command == "R" || command == "r") {
      string seed_str;
      cout << "Enter a non-negative integer to seed the random number "
              "generator: ";
      getline(cin, seed_str);
      Random::seed(stoi(seed_str));
    }

    cout << endl;

  } while (!(command == "x" || command == "X") && !cin.eof());

  return 0;
}

void printMenu() {
  cout << "Ciphers Menu" << endl;
  cout << "------------" << endl;
  cout << "C - Encrypt with Caesar Cipher" << endl;
  cout << "D - Decrypt Caesar Cipher" << endl;
  cout << "E - Compute English-ness Score" << endl;
  cout << "A - Apply Random Substitution Cipher" << endl;
  cout << "S - Decrypt Substitution Cipher from Console" << endl;
  cout << "F - Decrypt Substitution Cipher from File" << endl;
  cout << "R - Set Random Seed for Testing" << endl;
  cout << "X - Exit Program" << endl;
}

// "#pragma region" and "#pragma endregion" group related functions in this file
// to tell VSCode that these are "foldable". You might have noticed the little
// down arrow next to functions or loops, and that you can click it to collapse
// those bodies. This lets us do the same thing for arbitrary chunks!
#pragma region CaesarEnc

char rot(char c, int amount) {
  if (!isalpha((unsigned char)c)) return c;

  int shift = amount % 26;
  if (shift < 0) shift += 26;

  char up = toupper((unsigned char)c);
  int idx = up - 'A';
  int newIdx = (idx + shift) % 26;

  return ALPHABET[newIdx];
}

string rot(const string& line, int amount) {
  string result = "";

  for (int i = 0; i < (int)line.size(); i++) {
    char c = line[i];

    if (isalpha((unsigned char)c)) {
      result += rot(c, amount);
    } else if (c == ' ') {
      result += ' ';
    } else {
    }
  }

  return result;
}

void caesarEncryptCommand() {
  string text;
  string numRot;

  cout << "Enter the text to encrypt:" << endl;
  getline(cin, text);

  cout << "Enter the number of characters to rotate by:" << endl;
  getline(cin, numRot);

  int amount = stoi(numRot);
  cout << rot(text, amount) << endl;
}

#pragma endregion CaesarEnc

#pragma region CaesarDec

void rot(vector<string>& strings, int amount) {
  for (int i = 0; i < strings.size(); i++) {
    strings.at(i) = rot(strings.at(i), amount);
  }
}

string clean(const string& s) {
  string newString;
  for (int i = 0; i < s.size(); i++) {
    if (isalpha(s.at(i))) {
      newString += toupper(s.at(i));
    }
  }
  return newString;
}

vector<string> splitBySpaces(const string& s) {
  vector<string> words;
  string word;

  for (char c : s) {
    if (c != ' ') {
      word += c;
    } else {
      if (!word.empty()) {
        words.push_back(word);
        word.clear();
      }
    }
  }

  if (!word.empty()) {
    words.push_back(word);
  }

  return words;
}

string joinWithSpaces(const vector<string>& words) {
  string sentence;
  for (int i = 0; i < words.size(); i++) {
    sentence += words[i];
    if (i + 1 < words.size()) {
      sentence += " ";
    }
  }
  return sentence;
}

int numWordsIn(const vector<string>& words, const vector<string>& dict) {
  int numWords = 0;
  for (int i = 0; i < dict.size(); i++) {
    for (int k = 0; k < words.size(); k++) {
      if (dict[i] == words[k]) {
        numWords++;
      }
    }
  }
  return numWords;
}

void caesarDecryptCommand(const vector<string>& dict) {
  cout << "Enter the text to Caesar decrypt:" << endl;

  string line;
  getline(cin, line);

  bool foundAny = false;

  for (int shift = 0; shift < 26; shift++) {
    vector<string> words = splitBySpaces(line);

    rot(words, shift);

    vector<string> cleanedWords;
    cleanedWords.reserve(words.size());
    for (int i = 0; i < (int)words.size(); i++) {
      cleanedWords.push_back(clean(words[i]));
    }

    int matches = numWordsIn(cleanedWords, dict);

    if (!cleanedWords.empty() && matches > (int)cleanedWords.size() / 2) {
      cout << joinWithSpaces(words) << endl;
      foundAny = true;
    }
  }

  if (!foundAny) {
    cout << "No good decryptions found" << endl;
  }
}

#pragma endregion CaesarDec

#pragma region SubstEnc

string applySubstCipher(const vector<char>& cipher, const string& s) {
  string result;

  for (char c : s) {
    if (isalpha(c)) {
      result += cipher[toupper(c) - 'A'];
    } else {
      result += c;
    }
  }

  return result;
}

void applyRandSubstCipherCommand() {
  string line;
  getline(cin, line);

  vector<char> cipher = genRandomSubstCipher();

  string encrypted = applySubstCipher(cipher, line);

  cout << encrypted << endl;
}

#pragma endregion SubstEnc

#pragma region SubstDec

double scoreString(const QuadgramScorer& scorer, const string& s) {
  double score = 0.0;
  string quad;
  for (int i = 0; i <= s.size() - 4; i++) {
    quad = s.substr(i, 4);
    score += scorer.getScore(quad);
  }

  return score;
}

void computeEnglishnessCommand(const QuadgramScorer& scorer) {
  cout << "Enter a string for englishness scoring:" << endl;

  string input;
  getline(cin, input);

  input = clean(input);
  cout << scoreString(scorer, input) << endl;
}

vector<char> hillClimb(const QuadgramScorer& scorer, const string& ciphertext) {
  string ct = clean(ciphertext);

  if (ct.size() < 4) {
    return genRandomSubstCipher();
  }

  vector<char> bestKey = genRandomSubstCipher();

  string bestPlain = "";
  for (int i = 0; i < (int)ct.size(); i++) {
    bestPlain += bestKey[ct[i] - 'A'];
  }
  double bestScore = scoreString(scorer, bestPlain);

  int failures = 0;

  while (failures < 1000) {
    vector<char> newKey = bestKey;

    int a = Random::randInt(25);
    int b = Random::randInt(25);
    while (b == a) {
      b = Random::randInt(25);
    }

    char temp = newKey[a];
    newKey[a] = newKey[b];
    newKey[b] = temp;

    string newPlain = "";
    for (int i = 0; i < (int)ct.size(); i++) {
      newPlain += newKey[ct[i] - 'A'];
    }
    double newScore = scoreString(scorer, newPlain);

    if (newScore > bestScore) {
      bestKey = newKey;
      bestScore = newScore;
      failures = 0;
    } else {
      failures++;
    }
  }

  return bestKey;
}

void decryptSubstCipherCommand(const QuadgramScorer& scorer) {
  string ciphertext;
  getline(cin, ciphertext);

  for (int i = 0; i < (int)ciphertext.size(); i++) {
    if (isalpha(ciphertext[i])) {
      ciphertext[i] = toupper(ciphertext[i]);
    }
  }

  string ct = clean(ciphertext);
  if (ct.size() < 4) {
    vector<char> key = genRandomSubstCipher();
    cout << applySubstCipher(key, ciphertext) << endl;
    return;
  }

  vector<char> bestKey;
  double bestScore = 0.0;

  for (int run = 0; run < 25; run++) {
    vector<char> key = hillClimb(scorer, ciphertext);

    string plain = "";
    for (int i = 0; i < (int)ct.size(); i++) {
      plain += key[ct[i] - 'A'];
    }
    double sc = scoreString(scorer, plain);

    if (sc > bestScore) {
      bestScore = sc;
      bestKey = key;
    }
  }

  cout << applySubstCipher(bestKey, ciphertext) << endl;
}

void decryptSubstCipherFileCommand(const QuadgramScorer& scorer) {
  string inputFilename, outputFilename;
  getline(cin, inputFilename);
  getline(cin, outputFilename);
  ifstream in(inputFilename);
  string ciphertext = "";
  char ch;
  while (in.get(ch)) {
    ciphertext += ch;
  }
  in.close();

  for (int i = 0; i < (int)ciphertext.size(); i++) {
    if (isalpha(ciphertext[i])) {
      ciphertext[i] = toupper(ciphertext[i]);
    }
  }

  string ct = clean(ciphertext);
  vector<char> bestKey;
  double bestScore = 0.0;

  if (ct.size() < 4) {
    bestKey = genRandomSubstCipher();
  } else {
    for (int run = 0; run < 25; run++) {
      vector<char> key = hillClimb(scorer, ciphertext);

      string plain = "";
      for (int i = 0; i < (int)ct.size(); i++) {
        plain += key[ct[i] - 'A'];
      }
      double sc = scoreString(scorer, plain);

      if (sc > bestScore) {
        bestScore = sc;
        bestKey = key;
      }
    }
  }

  string plaintext = applySubstCipher(bestKey, ciphertext);

  ofstream out(outputFilename);
  out << plaintext;
  out.close();
}

#pragma endregion SubstDec
