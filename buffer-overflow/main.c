/*
 * Compile: gcc -o main main.c -g -std=c99 -fno-stack-protector
 * Add -DPREVENT_BUFFER_OVERFLOW for the fix
 *
 * This is a slightly modified version of:
 *
 * bf_vuln.c
 * This program is vulnerable to buffer overflow
 * and is solely for demonstrating purposes
 * Author: Nik Alleyne < nikalleyne at gmail dot com >
 * blog: http://securitynik.blogspot.com
 * Date: 2017-01-01
 */

#include <stdio.h>
#include <unistd.h>

void arbitrary_code()
{
    printf(" Now you know I should not be seen ! \n");
    printf(" But I am. Don't believe me just watch \n");
}

void get_input()
{
    char buffer[8];
#ifdef PREVENT_BUFFER_OVERFLOW
    fgets(buffer, 8, stdin);
#else
    gets(buffer);
#endif
    puts(buffer);
}

int benign_function()
{
    int x, y, z;
    x=3;
    y=4;
    z=x+y;
    return z;
}

int main()
{
    int a;
    a = benign_function();
    get_input();
    return 0;
}

/* Note: Nowhere in the above code did we call 'arbitrary_code'
 * However, we will see shortly that we can exectue this code
 * by using its memory location
 */
