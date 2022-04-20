#include <stdio.h>
#include <terminal/terminal.h>

bool is_prime(size_t number)
{
    for(size_t i = 2; i < number; i++)
    {
        if(number % i == 0)
        {
            return false;
        }
    }
    return true;
}

terminal s_term{"terminal_1"};

int main(int argc, char** argv)
{
    set_stdout([](const char* buf, size_t size, void* impl) {
        s_term.print_string(buf, size);
               });

    size_t number = 2;
    while(true)
    {
        if(is_prime(number))
        {
            printf("%d\n", number);
        }
        number++;
    }

    return 0;
}