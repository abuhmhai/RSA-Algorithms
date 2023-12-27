
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define ACCURACY 5
#define SINGLE_MAX 10000
#define EXPONENT_MAX 1000
#define BUF_SIZE 1024

/**
 * Tính a^b mod c
 */
int modpow(long long a, long long b, int c) {
    int res = 1;
    while(b > 0) {
        /* Cần nhân dài hạn để tránh tràn số... */
        if(b & 1) {
            res = (res * a) % c;
        }
        b = b >> 1;
        a = (a * a) % c; /* Phép nhân dài hạn ở đây để tránh tràn số */
    }
    return res;
}

/**
 * Tính ký hiệu Jacobi, (a, n)
 */
int jacobi(int a, int n) {
    int twos, temp;
    int mult = 1;
    while(a > 1 && a != n) {
        a = a % n;
        if(a <= 1 || a == n) break;
        twos = 0;
        while(a % 2 == 0 && ++twos) a /= 2; /* Loại bỏ các ước của 2 */
        if(twos > 0 && twos % 2 == 1) mult *= (n % 8 == 1 || n % 8 == 7) * 2 - 1;
        if(a <= 1 || a == n) break;
        if(n % 4 != 1 && a % 4 != 1) mult *= -1; /* Hệ số để đảo ngược */
        temp = a;
        a = n;
        n = temp;
    }
    if(a == 0) return 0;
    else if(a == 1) return mult;
    else return 0; /* a == n => gcd(a, n) != 1 */
}

/**
 * Kiểm tra xem a có phải là một chứng nhận Euler cho n không
 */
int solovayPrime(int a, int n) {
    int x = jacobi(a, n);
    if(x == -1) x = n - 1;
    return x != 0 && modpow(a, (n - 1)/2, n) == x;
}

/**
 * Kiểm tra xem n có thể là số nguyên tố, sử dụng độ chính xác là k (k thử nghiệm Solovay)
 */
int probablePrime(int n, int k) {
    if(n == 2) return 1;
    else if(n % 2 == 0 || n == 1) return 0;
    while(k-- > 0) {
        if(!solovayPrime(rand() % (n - 2) + 2, n)) return 0;
    }
    return 1;
}

/**
 * Tìm một số nguyên tố ngẫu nhiên (có thể) giữa 3 và n - 1, phân phối này không đồng đều,
 * xem prime gaps để biết thêm chi tiết
 */
int randPrime(int n) {
    int prime = rand() % n;
    n += n % 2; /* n cần là số chẵn để giữ tính lẻ sau khi chia modulo */
    prime += 1 - prime % 2;
    while(1) {
        if(probablePrime(prime, ACCURACY)) return prime;
        prime = (prime + 2) % n;
    }
}

/**
 * Tính gcd(a, b)
 */
int gcd(int a, int b) {
    int temp;
    while(b != 0) {
        temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

/**
 * Tìm một số mũ ngẫu nhiên x giữa 3 và n - 1 sao cho gcd(x, phi) = 1,
 * phân phối này cũng không đồng đều
 */
int randExponent(int phi, int n) {
    int e = rand() % n;
    while(1) {
        if(gcd(e, phi) == 1) return e;
        e = (e + 1) % n;
        if(e <= 2) e = 3;
    }
}

/**
 * Tính n^-1 mod m bằng phương pháp Euclid mở rộng
 */
int inverse(int n, int modulus) {
    int a = n, b = modulus;
    int x = 0, y = 1, x0 = 1, y0 = 0, q, temp;
    while(b != 0) {
        q = a / b;
        temp = a % b;
        a = b;
        b = temp;
        temp = x; x = x0 - q * x; x0 = temp;
        temp = y; y = y0 - q * y; y0 = temp;
    }
    if(x0 < 0) x0 += modulus;
    return x0;
}

/**
 * Đọc tệp fd thành một mảng byte sẵn sàng cho việc mã hóa.
 * Mảng sẽ được đệm bằng số không cho đến khi nó chia hết cho số lượng
 * byte được mã hóa trên mỗi khối. Trả về số byte đã đọc.
 */
int readFile(FILE* fd, char** buffer, int bytes) {
    int len = 0, cap = BUF_SIZE, r;
    char buf[BUF_SIZE];
    *buffer = malloc(BUF_SIZE * sizeof(char));
    while((r = fread(buf, sizeof(char), BUF_SIZE, fd)) > 0) {
        if(len + r >= cap) {
            cap *= 2;
            *buffer = realloc(*buffer, cap);
        }
        memcpy(&(*buffer)[len], buf, r);
        len += r;
    }
    /* Đệm khối cuối bằng số không để báo hiệu kết thúc mã hóa. Một khối bổ sung được thêm nếu không có chỗ trống */
    if(len + bytes - len % bytes > cap) *buffer = realloc(*buffer, len + bytes - len % bytes);
    do {
        (*buffer)[len] = '\0';
        len++;
    }
    while(len % bytes != 0);
    return len;
}

/**
 * Mã

 hóa thông điệp m bằng cách sử dụng số mũ công khai và modulus, c = m^e mod n
 */
int encode(int m, int e, int n) {
    return modpow(m, e, n);
}

/**
 * Giải mã cryptogram c bằng cách sử dụng số mũ cá nhân và modulus công khai, m = c^d mod n
 */
int decode(int c, int d, int n) {
    return modpow(c, d, n);
}

/**
 * Mã hóa thông điệp có chiều dài cho trước, sử dụng khóa công khai (số mũ, modulus)
 * Mảng kết quả sẽ có kích thước len/bytes, mỗi chỉ số là sự mã hóa
 * của "bytes" ký tự liên tiếp, được xác định bởi m = (m1 + m2*128 + m3*128^2 + ..),
 * encoded = m^exponent mod modulus
 */
int* encodeMessage(int len, int bytes, char* message, int exponent, int modulus) {
    int *encoded = malloc((len/bytes) * sizeof(int));
    int x, i, j;
    for(i = 0; i < len; i += bytes) {
        x = 0;
        for(j = 0; j < bytes; j++) x += message[i + j] * (1 << (7 * j));
        encoded[i/bytes] = encode(x, exponent, modulus);
#ifndef MEASURE
        printf("%d ", encoded[i/bytes]);
#endif
    }
    return encoded;
}

/**
 * Giải mã cryptogram có chiều dài cho trước, sử dụng khóa cá nhân (số mũ, modulus)
 * Mỗi gói được mã hóa nên đại diện cho "bytes" ký tự như encodeMessage đã mô tả.
 * Thông điệp trả về sẽ có kích thước len * bytes.
 */
int* decodeMessage(int len, int bytes, int* cryptogram, int exponent, int modulus) {
    int *decoded = malloc(len * bytes * sizeof(int));
    int x, i, j;
    for(i = 0; i < len; i++) {
        x = decode(cryptogram[i], exponent, modulus);
        for(j = 0; j < bytes; j++) {
            decoded[i*bytes + j] = (x >> (7 * j)) % 128;
#ifndef MEASURE
            if(decoded[i*bytes + j] != '\0') printf("%c", decoded[i*bytes + j]);
#endif
        }
    }
    return decoded;
}

/**
 * Phương thức chính để thử nghiệm hệ thống. Thiết lập các số nguyên tố p, q và tiếp tục mã hóa và
 * giải mã thông điệp được chỉ định trong "text.txt"
 */
int main(void) {
    int p, q, n, phi, e, d, bytes, len;
    int *encoded, *decoded;
    char *buffer;
    FILE *f;
    srand(time(NULL));
    while(1) {
        p = randPrime(SINGLE_MAX);
        printf("số nguyên tố đầu tiên, p = %d ... ", p);
        getchar();

        q = randPrime(SINGLE_MAX);
        printf("số nguyên tố thứ hai, q = %d ... ", q);
        getchar();

        n = p * q;
        printf("số modulus, n = pq = %d ... ", n);
        if(n < 128) {
            printf("Modulus nhỏ hơn 128, không thể mã hóa từng byte. Thử lại ... ");
            getchar();
        }
        else break;
    }
    if(n >> 21) bytes = 3;
    else if(n >> 14) bytes = 2;
    else bytes = 1;
    getchar();

    phi = (p - 1) * (q - 1);
    printf("hàm totient, phi = %d ... ", phi);
    getchar();

    e = randExponent(phi, EXPONENT_MAX);
    printf("Chọn số mũ công khai, e = %d\nKhóa công khai là (%d, %d) ... ", e, e, n);
    getchar();

    d = inverse(e, phi);
    printf("Tính số mũ cá nhân, d = %d\nKhóa cá nhân là (%d, %d) ... ", d, d, n);
    getchar();

    printf("Mở tệp \"text.txt\" để đọc\n");
    f = fopen("text.txt", "r");
    if(f == NULL) {
        printf("Không thể mở tệp \"text.txt\". Liệu nó có tồn tại không?\n");
        return EXIT_FAILURE;
    }
    len = readFile(f, &buffer, bytes); /* len sẽ là bội số của bytes, để gửi các khối nguyên byte */
    fclose(f);

    printf("Tệp \"text.txt\" đã đọc thành công, đã đọc %d byte. Mã hóa luồng byte thành các khối %d byte ... ", len, bytes);
    getchar();
    encoded = encodeMessage(len, bytes, buffer, e, n);
    printf("\nMã hóa hoàn tất thành công ... ");
    getchar();

    printf("Giải mã thông điệp đã mã hóa ... ");
    getchar();
    decoded = decodeMessage(len/bytes, bytes, encoded, d, n);

    printf("\nHoàn tất thuật toán mã hoá RSA!\n");

    free(encoded);
    free(decoded);
    free(buffer);
    return EXIT_SUCCESS;
}
