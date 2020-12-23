#include "page.h"
#include "process.h"
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <deque>
#include <vector>
#include <queue>
#include <list>
#include <iostream>
#include <sstream>
#include <fstream>

#define MEMORYSIZE 512

using namespace std;

// pSize анализируется из pList с указанием каждого идентификатора процесса и его размера << pid, size> ...>
vector<vector<int> > pSize;
// pTrace анализируется из pTrace, который отслеживает все ячейки памяти, на которые имеются ссылки, вызываемые системой << pid, memoryLocation> ...>
vector<vector<int> > pTrace;
// processList представляет все текущие процессы в нашей симуляции и представляет собой вектор объектов процесса, созданных в начале программы.
vector<Process*> processList;
// mainMemory представляет основную память в этой симуляции, она реализована как двухсторонняя очередь, потому что нам нужно будет удалить с обоих концов для некоторых алгоритмов
deque<deque<Page*> > mainMemory;
// virtualMemory представляет виртуальную память в этой симуляции, она реализована как вектор вектора страниц (pageTable), который будет содержать таблицу страниц каждого процесса
vector<vector<Page*> > virtualMemory;
/ pageSwap будет вести подсчет количества замен страниц, выполненных при моделировании
int pageSwaps = 0;


// getNextPageNotInMem - это вспомогательная функция для подготовки к поиску следующей страницы не в памяти
// Дано: идентификатор процесса, запрошенного ptrace, страница, запрошенная ptrace, и размер страниц
// Возвращает: индекс следующей страницы не в памяти, -1, если следующая страница не в памяти не найдена
int getNextPageNotInMem(int pid, int page, int sizeOfPages) {
	cout << " Получение страницы не в памяти! " << endl;
	bool foundNextPage = false ;	// мы продолжаем поиск, если foundNextPage не удовлетворен или если мы просмотрели всю таблицу страниц
	int currPageIndex = page + 1 ;	// мы начинаем поиск со страницы после того, как текущая страница вставлена ​​в основную память
	int lastPageIndex = ((pSize[pid][ 1 ]) - 1 ) / sizeOfPages;	// мы отслеживаем последний индекс, чтобы знать, когда прекратить поиск

	// продолжаем поиск, пока не найдем страницу не в памяти 
	while (foundNextPage == false) {
		/*
		cout << "Страница: " << pSize[pid][1] << endl;
		cout << "Выполняется поиск на странице: " << currPageIndex << endl;
		cout << "Индекс последней страницы: " << lastPageIndex << endl;
		*/
		// если мы просмотрели всю таблицу страниц, мы возвращаем -1
		if (currPageIndex > lastPageIndex) {
			return -1;
		}
		/ если действительный бит равен 0, значит, мы нашли следующую страницу не в памяти
		if (virtualMemory[pid][currPageIndex]->validBit == 0) {
			return currPageIndex;	// возвращаем индекс следующей страницы не в памяти
		}
		// cout << "Текущая страница уже в памяти" << endl;
		currPageIndex++;
	}
	cout << "Поиск завершен" << endl;
	return -1;	// если мы не нашли следующую страницу не в памяти, возвращаем -1
}

void FIFO(int sizeOfPages, int numFrames) {
	cout << " В FIFO! " << endl;
	int pid;	// pid - это идентификатор процесса, который в настоящее время вызывает "трассировка"
	int memLoc;	// memLoc - это место в памяти, которое "trace" в настоящее время вызывает
   	int mruLoc;	// freeLoc - это индекс фрейма в основной памяти страницы, которую "trace" в настоящее время вызывает, если она уже находится в памяти
   	Page* reqPage;	// reqPage - это страница, которую "трассировка" в настоящий момент запрашивает
    
    for ( int i = 0 ; i <pTrace. size (); i ++) {
        pid = pTrace[i][ 0 ];	     // получаем идентификатор процесса
        memLoc = pTrace[i][ 1 ];	// получить место в памяти, вычислить количество кадров на процесс
        reqPage = virtualMemory[pid][(memLoc- 1 ) / sizeOfPages];	// мы выполняем memLoc-1, потому что память начинается с 1, а наш индекс начинается с 0, поэтому мы сдвигаем его на 1, вычитая 1

        // ОТЛАДКА ПЕЧАТНЫХ ОТЧЕТОВ
        cout << "ОТЛАДКА FIFO: Размер страниц: " << sizeOfPages << endl;
        cout << "ОТЛАДКА FIFO: Номер итерации: " << i << endl;
        cout << "ОТЛАДКА FIFO: Расположение памяти: " << memLoc << endl;
        cout << "ОТЛАДКА FIFO: размер процесса: " << pSize[pid][1] << endl;
        cout << "Получение pid: " << pid << " запрашивающего номер страницы: " << (memLoc-1)/sizeOfPages << endl;

        // Сначала нам нужно проверить, находится ли эта страница уже в памяти или нет, если нет, то нам нужно поместить ее в память
        if (reqPage-> validBit == 0 ) {
            cout << " Страницы нет в памяти! " << endl;
            // Если есть свободное место, мы можем просто поместить страницу в память, в противном случае нам нужно заменить фрейм и выполнить замену страниц
            if (mainMemory[pid].size () <numFrames) {
                mainMemory[pid].push_back (reqPage);    // помещаем reqPage в основную память
            }
            // Если свободного места нет, мы выдвинем переднюю часть, потому что при инициализации загруженных страниц основной памяти с кадра 0 до кадра numFrames,
            // так что наименее использованная страница всегда будет первым кадром
            else {
                mainMemory[pid].front () -> validBit = 0 ;   // устанавливаем страницу перед памятью равной 0, так как она больше не будет в памяти
                mainMemory[pid].pop_front ();   // вытаскиваем первый кадр, так как он вставляется первым
                mainMemory[pid].push_back(reqPage);   // помещаем недавно запрошенную страницу в основную память. Последняя страница будет сзади
                cout << " Страница заменена! " << endl;
                pageSwaps ++;    // увеличиваем счетчик смены страниц
            }
            reqPage-> validBit = 1 ;    // устанавливаем validBit of reqPage равным 1, потому что теперь он находится в памяти
        }
        // Если страница уже в памяти, ничего не делаем
        else {
            cout << " Страница уже в памяти! " << endl;
        }
        cout << endl;
    }
}

void pFIFO (int sizeOfPages, int numFrames) {
	int pid;     // идентификатор процесса
	int memLoc;	// запрошенная ячейка памяти
	int mruLoc;     // отслеживаем запрошенную ячейку памяти в памяти
	Page* reqPage;

	for (int i = 0; i < pTrace.size(); i++) {
		pid = pTrace[i][0];	// получаем идентификатор процесса
		memLoc = pTrace[i][1];	  // получаем запрошенную ячейку памяти
		//numFrames = (MEMORYSIZE/sizeOfPages)/processList.size();     // количество кадров, выделенных на процесс
		reqPage = virtualMemory[pid][(memLoc-1)/sizeOfPages];	  // получаем запрошенную страницу

		// ОТЛАДКА ПЕЧАТНЫХ ОТЧЕТОВ
		cout << "ОТЛАДКА pFIFO: Размер страниц: " << sizeOfPages << endl;
		cout << "ОТЛАДКА pFIFO: Номер итерации: " << i << endl;
		cout << "ОТЛАДКА pFIFO: Расположение памяти: " << memLoc << endl;
		cout << "ОТЛАДКА pFIFO: Размер процесса: " << pSize[pid][1] << endl;
		cout << "Получение pid: " << pid << " запрашивающего номер страницы: " << (memLoc-1)/sizeOfPages << endl;

		// затем мы проверяем, есть ли другая страница в vm не в мм
		int nextPageAvail = getNextPageNotInMem(pid, (memLoc-1)/sizeOfPages, sizeOfPages);
		cout << "Доступна следующая страница: " << nextPageAvail << endl;
		cout << "Количество кадров: " << numFrames << endl;
		cout << "Размер основной памяти по pid: " << pid << ": " << mainMemory[pid].size() << endl;
		// если запрошенной страницы нет в памяти, то нам нужно вставить ее в память
		if (reqPage->validBit == 0) {
			cout << "Не в памяти!" << endl;
			// если есть свободная память, мы можем поместить ее в память
			if (mainMemory[pid].size() < numFrames) {
				cout << "Свободное место, можно вставить в память" << endl;
				mainMemory[pid].push_back(reqPage);
				reqPage->validBit = 1;
			}
			// иначе нам нужно удалить первую страницу, так как она будет вставлена ​​первой, и вставить reqPage
			if (mainMemory[pid].size() >= numFrames) {
				cout << "Нет свободного места, выполняется замена страниц" << endl;
				mainMemory[pid].front()->validBit = 0;
				mainMemory[pid].pop_front();
				mainMemory[pid].push_back(reqPage);
				reqPage->validBit = 1;
				cout << "Переключенная страница" << endl;
				pageSwaps++;
			}
			// если следующей страницы нет в памяти, нам нужно вставить ее в память
			if (nextPageAvail != -1) {
				Page* nextReqPage = virtualMemory[pid][nextPageAvail];
				// если есть свободная память, мы можем поместить ее в память
				if (numFrames - mainMemory[pid].size() >= 1) {
					mainMemory[pid].push_back(nextReqPage);
					nextReqPage->validBit = 1;
				}
				// если свободной памяти нет, нам нужно открыть верхнюю часть основной памяти и поместить последнюю страницу в конец основной памяти
				else {
					mainMemory[pid].front()->validBit = 0;
					mainMemory[pid].pop_front();
					mainMemory[pid].push_back(nextReqPage);
					nextReqPage->validBit = 1;
					cout << "Swapped page" << endl;
					pageSwaps++;
				}
			}
		}
		else {
			// если он уже в памяти, ничего не делаем
		}
	}
}

int main(int argc, char * const argv[]) {
    
    int sizeOfPages;   // sizeOfPages представляет размер каждого кадра и страницы
    bool preage = false ;   // предварительная обработка - это логическое значение, которое представляет истину или ложь в зависимости от флага, предварительная обработка запроса включена по умолчанию
    строковый flag;   // flag представляет флаг, который вводится пользователем для выбора между подкачкой по запросу или подготовкой
    istringstream iss;   // iss - объект строкового потока для преобразования аргументов в строки
    
    // переменные аргумента
    int processId;  // processId представляет идентификатор процесса в списке
    int proMemSize; // proMemSize представляет объем памяти для процесса
    int proMemLoc;  // proMemLoc представляет собой ячейку памяти, к которой система запрашивает доступ
    
    // переменные инициализации
    int mainMemFrames;    // mainMemFrames представляет количество фреймов страницы в основной памяти
    int numFrames;      // numFrames представляет количество кадров страницы на процесс в основной памяти при инициализации
    
    // проверяем, что введено 6 аргументов
    if (argc! = 6 ) {
        cerr << " Ошибка: введите допустимое количество аргументов! " << endl;
        return  1 ;
    }

    // в противном случае, если количество аргументов правильное, запускаем анализ аргументов
    // захватить plist в качестве аргумента, если null, то предложит пользователю ввести файл plist
    ifstream in_1 (argv [ 1 ]);
    if (! (in_1)) {
        cerr << " Ошибка: введите действительный файл plist! " << endl;
        return  1 ;
    }

    // парсим через plist, сохраняя каждый идентификатор процесса и его размер памяти в векторе "process", затем помещаем его в pSize в форме <pid, memsize>
    while(in_1 >> processId >> proMemSize){
        vector<int> process;
        process.push_back(processId);
        process.push_back(proMemSize);
        pSize.push_back(process);
    }
    // получаем ptrace в качестве аргумента, если ноль, то запрашиваем у пользователя ввод файла ptrace
    ifstream in_2(argv[2]);
    if (!(in_2)) {
        cerr << "Ошибка: введите допустимый файл ptrace!" << endl;
        return 1;
    }

    // анализируем через ptrace, сохраняя серию обращений к памяти в векторе "memLocAccess", затем отправляем его в pTrace в форме <pid, memLocation>
    while(in_2 >> processId >> proMemLoc){
        vector<int> memLocAccess;
        memLocAccess.push_back(processId);
        memLocAccess.push_back(proMemLoc);
        pTrace.push_back(memLocAccess);
    }

    // проверяем корректность аргумента  (размер страниц), конвертируем его в целое число и сохраняем в переменной sizeOfPages
    sizeOfPages = atoi(argv[3]);
    if (sizeOfPages <= 0) {
        cerr << "Ошибка: введите допустимое положительное целое число для размера страниц!" << endl;
        return 1;
    }

    // проверяем правильность аргумента (флаг подготовки), сохраняем в переменной "flag" и проверяем, что флаг имеет строковый тип
    iss.str(argv[4]);
    if (!(iss >> flag)) {
        cerr << "Ошибка: введите допустимый аргумент для флага!" << endl;
        return 1;
    }
    // проверяем, является ли флаг одним из двух допустимых символов (+ или -), если нет, возвращаем ошибку
    if (((flag.compare("+") != 0) && (flag.compare("-")) != 0)) {
        cerr << "Ошибка: введите действительный флаг!" << endl;
        return 1;
    }
    if (flag.compare("+") == 0) {
        prepage = true;
    } else {
        prepage = false;
    }
    
    // перебираем все размеры процесса и создаем новый объект процесса и помещаем его в processList
    for (int i = 0; i < pSize.size(); i++) {
        proMemSize = pSize[i][1];
        processList.push_back(new Process(i, proMemSize, sizeOfPages));
    }
    
    // initialize virtual memory by pushing each process' page table to virtualMemory
    for(int i = 0; i < processList.size(); i++) {
        for (int j = 0; j < MEMORYSIZE/sizeOfPages/processList.size(); j++) {
            processList[i]->pageTable[j]->validBit = 1;
        }
        virtualMemory.push_back(processList[i]->pageTable);
    }
    
    // устанавливаем значения mainMemFrames и numFrames
    mainMemFrames = MEMORYSIZE/sizeOfPages;
    numFrames = mainMemFrames/processList.size();
    // инициализируем равное количество страниц для каждого процесса в основной памяти
    for(int i = 0; i < processList.size(); i++) {
        deque<Page*> frames;
        // для каждого процесса помещаем в «фреймы» одинаковое количество кадров из виртуальной памяти
        for(int j = 0; j < numFrames; j++) {
            frames.push_back(virtualMemory[i][j]);
        }
        // после того, как равное количество кадров было загружено в "frames", загружаем его в основную память
        mainMemory.push_back(frames);
        frames.clear();
    }
    
    
    if (prepage == false) {
            FIFO(sizeOfPages, numFrames);
            cout << "Свапы страниц " << pageSwaps << endl;
        }
    } else {
            pFIFO(sizeOfPages, numFrames);
            cout << "Свапы страниц " << pageSwaps << endl;
    }
}
