#include <limits.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static bool print(const char* data, size_t length) {
	const unsigned char* bytes = (const unsigned char*) data;
	for (size_t i = 0; i < length; i++)
		if (putchar(bytes[i]) == EOF)
			return false;
	return true;
}

/*int printf(const char* restrict format, ...) {
	va_list parameters;
	va_start(parameters, format);

	int written = 0;

	while (*format != '\0') {
		size_t maxrem = INT_MAX - written;

		if (format[0] != '%' || format[1] == '%') {
			if (format[0] == '%')
				format++;
			size_t amount = 1;
			while (format[amount] && format[amount] != '%')
				amount++;
			if (maxrem < amount) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, amount))
				return -1;
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format++;

		if (*format == 'c') {
			format++;
			char c = (char) va_arg(parameters, int);
			if (!maxrem) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(&c, sizeof(c)))
				return -1;
			written++;
		} else if (*format == 's') {
			format++;
			const char* str = va_arg(parameters, const char*);
			size_t len = strlen(str);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(str, len))
				return -1;
			written += len;
		} else {
			format = format_begun_at;
			size_t len = strlen(format);
			if (maxrem < len) {
				// TODO: Set errno to EOVERFLOW.
				return -1;
			}
			if (!print(format, len))
				return -1;
			written += len;
			format += len;
		}
	}

	va_end(parameters);
	return written;
}*/

int printf(const char* restrict format, ...)
{
	va_list parameters;
	va_start(parameters, format);

	int written = 0;
	size_t amount;
	bool rejected_bad_specifier = false;

	while ( *format != '\0' )
	{
		if ( *format != '%' )
		{
		print_c:
			amount = 1;
			while ( format[amount] && format[amount] != '%' )
				amount++;
			print(format, amount);
			format += amount;
			written += amount;
			continue;
		}

		const char* format_begun_at = format;

		if ( *(++format) == '%' )
			goto print_c;

		if ( rejected_bad_specifier )
		{
		incomprehensible_conversion:
			rejected_bad_specifier = true;
			format = format_begun_at;
			goto print_c;
		}

		if ( *format == 'c' )
		{
			format++;
			char c = (char) va_arg(parameters, int /* char promotes to int */);
			print(&c, sizeof(c));
		}
        else if ( *format == 'd' )
        {
            format++;
            int d = (int) va_arg(parameters, int);
            int size = 0;

            //Zero override
            if (d==0) {
                char buffer[1];
                buffer[0] = '0';
                print(buffer, 1);
            }

            /* Determine size of buffer */
            int temp = d;
            while(temp) { ++size; temp /= 10; }
            //if (temp == 0) {size = 1;}
            char buffer[size];


            /* Get each individual digit and add to buffer as char*/
            int i;
            for(i=0; d; i++)
            {
                buffer[size-i-1] = (d % 10) + '0';
                d /= 10;
            }

            print(buffer, size);
        }
        else if ( *format == 'u' )
        {
            format++;
            uint32_t d = (uint32_t) va_arg(parameters, uint32_t);
            uint32_t size = 0;

            //Zero override
            if (d==0) {
                char buffer[1];
                buffer[0] = '0';
                print(buffer, 1);
            }

            /* Determine size of buffer */
            uint32_t temp = d;
            while(temp) { ++size; temp /= 10; }
            //if (temp == 0) {size = 1;}
            char buffer[size];


            /* Get each individual digit and add to buffer as char*/
            uint32_t i;
            for(i=0; d; i++)
            {
                buffer[size-i-1] = (d % 10) + '0';
                d /= 10;
            }

            print(buffer, size);
        }
        else if ( *format == 'x' )
        {
            format++;
            uint32_t d = (uint32_t) va_arg(parameters, uint32_t);
            uint32_t size = 0;
            uint32_t divisor = 16;
            //Zero override
            if(d == 0) {
                char buffer[1];
                buffer[0] = '0';
                print(buffer, 1);
            }

            // deterine size!
            uint32_t temp = d;
            while(temp) { ++size; temp /= 16; }
            char buffer[size];

            int loop = 0;
            do {
                uint32_t remainder = d % divisor;

                buffer[loop++] = (char) ((remainder < 10) ? remainder + '0' : remainder + 'a' - 10);

            } while (d /= divisor);

            buffer[loop] = 0;

            // reverse array
            char tmp;
            char start = 0;
            char end = size - 1;
            while(start < end) {
                tmp = buffer[start];
                buffer[start] = buffer[end];
                buffer[end] = tmp;
                start++;
                end--;
            }

            print(buffer, size);
        }
        else if ( *format == 'l' && *(format+1) == 'x')
        {
            format = format + 2;
            uint64_t d = (uint64_t) va_arg(parameters, uint64_t);
            uint64_t size = 0;
            uint64_t divisor = 16;
            //Zero override
            if(d == 0) {
                char buffer[1];
                buffer[0] = '0';
                print(buffer, 1);
            }

            // deterine size!
            uint64_t temp = d;
            while(temp) { ++size; temp /= 16; }
            char buffer[size];

            int loop = 0;
            do {
                uint64_t remainder = d % divisor;

                buffer[loop++] = (char) ((remainder < 10) ? remainder + '0' : remainder + 'a' - 10);

            } while (d /= divisor);

            buffer[loop] = 0;

            // reverse array
            char tmp;
            char start = 0;
            char end = size - 1;
            while(start < end) {
                tmp = buffer[start];
                buffer[start] = buffer[end];
                buffer[end] = tmp;
                start++;
                end--;
            }

            print(buffer, size);
        }
		else if ( *format == 's' )
		{
			format++;
			const char* s = va_arg(parameters, const char*);
			print(s, strlen(s));
		}
		else
		{
			goto incomprehensible_conversion;
		}
	}

	va_end(parameters);

	return written;
}