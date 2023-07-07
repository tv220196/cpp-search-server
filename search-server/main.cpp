// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <iostream>
#include <string>
using namespace std;

int main() {
	int incl_3 = 0;
	for (int i = 1; i <= 1000; ++i) {
		string num = to_string(i);
		bool flag = false;
		for (const char& c : num) {
			if (c == '3') {
				flag = true;
			}
		}
		if (flag) {
			++incl_3;
		}
	}
	cout << incl_3 << endl;
}
// Закомитьте изменения и отправьте их в свой репозиторий.
