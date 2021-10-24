#include<stdio.h>
#include <graphics/graphics.h>

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

int main(int argc, char** argv)
{
    initialize_text_mode(90, 30);

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