/*
 * Cryptographic API
 *
 * RSA cipher algorithm implementation
 *
 * Copyright (c) 2007 Tasos Parisinos <t.parisinos@sciensis.com>
 * Copyright (c) 2007 Sciensis Advanced Technology Systems
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <crypto/rsa.h>

#define AUX_OPERAND_COUNT	4
#define U32_MAX		        0xFFFFFFFF
#define U32_MSB_SET		    0x80000000
#define LEFTOP			    aux[0]
#define RIGHTOP		        aux[1]

// #define RSA_DEBUG
/* Log options */
#ifdef RSA_DEBUG
#define RSA_LOG(fmt, arg...) \
    do { \
        printk("RSA: In %s "fmt, __func__, ##arg); \
    } while (0)
#define RSA_DUMP(info, data, len) \
    do { \
        u_long i = 0; \
        printk("RSA: In %s "info, __func__); \
        printk("\n"); \
        for (i = 0; i < len; i++) \
            printk("0x%x ", data[i]); \
        printk("\n"); \
    } while (0)
#else
#define RSA_LOG(...) do{}while(0)
#define RSA_DUMP(...) do{}while(0)
#endif


static int rsa_op_resize(rsa_op **n, int size)
{
	int retval = 0;
	rsa_op *handle = *n;

	/* If there is no allocated rsa_op passed in, allocate one */
	if (!handle)
		return rsa_op_alloc(n, size);

	/* If the rsa_op passed in has the available limbs */
	if (handle->limbs >= size) {
		/* Zero the xtra limbs */
		if (size < handle->size)
			memset(handle->data + size, 0,
			       (handle->size - size) * sizeof(u32));
		handle->size = size;
	}
	else {
		rsa_op *tmp = NULL;

		retval = rsa_op_alloc(&tmp, size);
		if (retval < 0)
			return retval;

		/* Copy the original data */
		memcpy(tmp->data, handle->data, handle->size * sizeof(u32));
		tmp->sign = handle->sign;
		rsa_op_free(*n);
		*n = tmp;
	}

	return retval;
}

/* Count the leading zeroes of an operand */
static u32 rsa_op_clz(rsa_op *n)
{
	int i;
	u32 limb, retval = 0;

	for (i = n->size - 1; i >= 0; i--) {
		limb = n->data[i];

		if (!limb) {
			retval += 32;
			continue;
		}

		while (!(limb & U32_MSB_SET)) {
			retval++;
			limb = limb << 1;
		}
		break;
	}

	return retval;
}

/* 
 * rsa_digits - get the significant digits
 */
static unsigned int rsa_digits (rsa_op *op, unsigned int digits)
{
    u_long *a = op->data;
    
	if(digits) {
		digits--;

		do {
			if(*(a+digits))
				break;
		}while(digits--);

		return(digits + 1);
	}

	return(digits);
}

static int rsa_op_compare(rsa_op *l, rsa_op *r)
{
	u32 *lbuf, *rbuf;
    u32 n_digits;

#define RSA_CMP_STEP(...) do{}while(0)

    RSA_CMP_STEP("Step 1 l sign %d, r sign %d\n", l->sign, r->sign);
	/* Compare the two operands based on sign */
	if (l->sign != r->sign)
		return (l->sign)? -1: 1;

    RSA_CMP_STEP("Step 2 l size %d, r size %d\n", l->size, r->size);
	/* Compare the two operands based on their size */
	if (l->size > r->size && rsa_op_clz(l) < (l->size - r->size) * 32)
		return 1;
	else if (r->size > l->size && rsa_op_clz(r) < (r->size - l->size) * 32)
		return -1;

    RSA_CMP_STEP("Step 3\n");
	/* Compare the two operands based on their hex values */
	lbuf = l->data;
	rbuf = r->data;
    n_digits = rsa_digits(r, r->size);

    if(n_digits) {
		do {
			n_digits--;
			if (*(lbuf + n_digits) > *(rbuf + n_digits))
				return (1);
			if (*(lbuf + n_digits) < *(rbuf + n_digits))
				return (-1);
		}while(n_digits);
	}
    
    RSA_CMP_STEP("Step 4\n");
#undef RSA_CMP_STEP
	return 0;
}

static void rsa_op_complement(rsa_op *n)
{
	int i, s;
	u32 *buf;

	s = n->size;
	buf = n->data;
	for (i = 0; i < s; i++)
		buf[i] = ~buf[i];

	/* Add 1 using the addition carry */
	for (i = 0; i < s; i++) {
		buf[i] += 1;
		if (buf[i])
			break;
	}
}

static void rsa_op_canonicalize(rsa_op *n)
{
	int i;
	u32 *buf = n->data;

	for (i = n->size - 1; i >= 0; i--)
		if (!buf[i] && n->size > 1)
			n->size--;
		else
			break;
}

static int rsa_op_shift(rsa_op **n, int bits)
{
	int i, distance, size, lz, retval;
	u32 *buf;
	rsa_op *handle;

	if (!bits)
		return 0;
	handle = *n;

	/* Shifting to the right, no resize needed */
	if (bits > 0) {
		/* Drop off one limb for each 32 bit shift */
		distance = bits / 32;
		size = handle->size;
		buf = handle->data;
		for (i = 0; i < size; i++)
			buf[i] = (i + distance >= size)? 0: buf[i + distance];

		/* Shift the remaining 'bits' mod 32 */
		bits = bits % 32;
		if (bits) {
			size -= distance;
			distance = 32 - bits;
			for (i = 0; i < size; i++) {
				buf[i] = buf[i] >> bits;
				if (i < size - 1)
					buf[i] |=  buf[i + 1] << distance;
			}
		}

		rsa_op_canonicalize(handle);
		return 0;
	}

	bits = -bits;
	lz = rsa_op_clz(handle) + (handle->limbs - handle->size) * 32;

	/*
	 * Shifting to the left.
	 * Reallocation is needed when the leading zeroes are less than
	 * the shift distance
	 */
	if (lz < bits) {
		/* Compute the size of the reallocation */
		size = (bits - lz + 31) / 32;
		retval = rsa_op_resize(n, handle->limbs + size);
		if (retval < 0)
			return retval;
		handle = *n;
	}
	else
		handle->size += ((bits - rsa_op_clz(handle) + 31) / 32);

	buf = handle->data;
	distance = bits / 32;
	/* Shift data 1 byte to the left for each 32 bit shift */
	if (distance) {
		/* Shift bytes */
		for (i = handle->size - distance - 1; i >= 0; i--)
			buf[i + distance] = buf[i];

		/* Zero the shifted in bytes */
		memset(handle->data, 0, distance * sizeof(u32));
	}

	/* Shift the remaining 'bits' mod 32 */
	bits = bits % 32;
	distance = 32 - bits;
	if (bits)
		for (i = handle->size - 1; i >= 0; i--) {
			buf[i] = buf[i] << bits;
			if (i > 0)
				buf[i] |= (buf[i - 1] >> distance);
		}

	return 0;
}

/*
 * Compute the modular inverse of an operand using only its 32 least
 * significant bits (single presicion modular inverse)
 */
static u32 rsa_op_modinv32(rsa_op *n)
{
	u32 i, x, y, tmp, pow1;

	pow1 = y = 1;
	x = n->data[0];

	for (i = 2; i <= 32; i++) {
		pow1 = pow1 << 1;

		/*
		 * This is the computation of x * y mod r, only r is a power
		 * of 2, specifically 2^i so the modulo is computed by
		 * keeping the least i bits of x * y.
		 */
		tmp = ((u64)x * y) & (U32_MAX >> (32 - i));
		if (pow1 < tmp)
			y += pow1;
	}

	y = ~y + 1;
	return y;
}

static int rsa_product(rsa_op **res, rsa_op *l, rsa_op *r, rsa_op **aux)
{
	int i, j, ls, rs, retval;
	u32 *buf, *lbuf, *rbuf;
	u64 tmp;
	rsa_op *handle;

	retval = rsa_op_copy(&LEFTOP, l);
	if (retval < 0)
		return retval;

	retval = rsa_op_copy(&RIGHTOP, r);
	if (retval < 0)
		return retval;

	ls = l->size;
	rs = r->size;
	retval = rsa_op_resize(res, ls + rs);
	if (retval < 0)
		return retval;

	handle = *res;
	memset(handle->data, 0, handle->size * sizeof(u32));
	handle->sign = LEFTOP->sign ^ RIGHTOP->sign;

	buf = handle->data;
	lbuf = LEFTOP->data;
	rbuf = RIGHTOP->data;

	/* Make the multiplication, using the standard algorithm */
	for (i = 0; i < rs; i++) {
		tmp = 0;
		for (j = 0; j < ls; j++) {
			tmp = buf[i + j] + (lbuf[j] * (u64)rbuf[i]) +
			      (tmp >> 32);
			buf[i + j] = (u32)tmp;
		}

		buf[i + ls] = tmp >> 32;
	}

	rsa_op_canonicalize(handle);
	return 0;
}

static int rsa_difference(rsa_op **res, rsa_op *l, rsa_op *r, rsa_op **aux)
{
	int i, size, retval;
	u32 *buf, *lbuf, *rbuf;
	rsa_op *handle;

	retval = rsa_op_copy(&LEFTOP, l);
	if (retval < 0)
		return retval;

	retval = rsa_op_copy(&RIGHTOP, r);
	if (retval < 0)
		return retval;

	size = max(l->size, r->size) + (l->sign != r->sign);
	retval = rsa_op_resize(res, size);
	if (retval < 0)
		return retval;

	retval = rsa_op_resize(&LEFTOP, size);
	if (retval < 0)
		return retval;

	retval = rsa_op_resize(&RIGHTOP, size);
	if (retval < 0)
		return retval;

	handle = *res;
	buf = handle->data;
	lbuf = LEFTOP->data;
	rbuf = RIGHTOP->data;

	/* If the operands are both negative or positive perform subtraction */
	if (LEFTOP->sign == RIGHTOP->sign) {
		bool borrow = false;
		u32 limb;

		for (i = 0; i < size; i++) {
			limb = borrow + rbuf[i];
			buf[i] = lbuf[i] - limb;
			borrow = (lbuf[i] < limb);
		}

		if (borrow) {
			rsa_op_complement(handle);
			handle->sign = !RIGHTOP->sign;
		}
		else
			handle->sign = RIGHTOP->sign;
	}
	/* If the operands are not signed in the same way perform addition */
	else {
		bool carry = false;
		u64 sum;

		for (i = 0; i < size; i++) {
			sum = lbuf[i] + rbuf[i] + carry;
			carry = (sum > U32_MAX);
			buf[i] = (u32)sum;
		}

		handle->sign = LEFTOP->sign;
	}

	rsa_op_canonicalize(handle);
	return 0;
}

static int rsa_remainder(rsa_op **res, rsa_op *l, rsa_op *r, rsa_op **aux)
{
	int i, k, retval;

	retval = rsa_op_copy(&LEFTOP, l);
	if (retval < 0)
		return retval;

	retval = rsa_op_copy(&RIGHTOP, r);
	if (retval < 0)
		return retval;

	k = (LEFTOP->size - RIGHTOP->size) * 32;
	k -= (rsa_op_clz(LEFTOP) - rsa_op_clz(RIGHTOP));

	/* Align the divisor to the dividend */
	retval = rsa_op_shift(&RIGHTOP, -k);
	if (retval < 0)
		return retval;

	for (i = 0; i <= k; i++) {
		retval = rsa_difference(res, LEFTOP, RIGHTOP, aux + 2);
		if (retval < 0)
			return retval;

		if (!(*res)->sign) {
			retval = rsa_op_copy(&LEFTOP, *res);
			if (retval < 0)
				return retval;
		}

		retval = rsa_op_shift(&RIGHTOP, 1);
		if (retval < 0)
			return retval;
	}

	rsa_op_canonicalize(LEFTOP);
	return rsa_op_copy(res, LEFTOP);
}

/* Compute the montgomery product of two numbers */
static int rsa_mproduct(rsa_op **res, rsa_op *l, rsa_op *r, rsa_op *n,
			u32 modinv, rsa_op **aux)
{
	int nsize, i, j, k, retval;
	u32 *buf, *nbuf, *tmp, m;
	u64 product = 0;

	nsize = n->size;
	k = nsize << 1;
	retval = rsa_product(res, l, r, aux);
	if (retval < 0)
		return retval;

	retval = rsa_op_resize(res, max((*res)->size, k));
	if (retval < 0)
		return retval;

	tmp = buf = (*res)->data;
	nbuf = n->data;

	for (i = 0; i < nsize; i++, tmp++) {
		m = buf[i] * modinv;
		product = 0;

		for (j = 0; j < nsize; j++) {
			product = tmp[j] + (m * (u64)nbuf[j]) + (product >> 32);
			tmp[j] = (u32)product;
		}

		for (j = nsize + i; j < k; j++) {
			product = buf[j] + (product >> 32);
			buf[j] = (u32)product;
		}
	}

	retval = rsa_op_resize(res, (*res)->size + 1);
	if (retval < 0)
		return retval;

	(*res)->data[(*res)->size - 1] = product >> 32;
	retval = rsa_op_shift(res, nsize * 32);
	if (retval < 0)
		return retval;

	if (rsa_op_compare(*res, n) >= 0) {
		retval = rsa_difference(res, *res, n, aux);
		if (retval < 0)
			return retval;
	}
	return 0;
}

/**
 * rsa_op_alloc - allocate an rsa_op
 * @n: pointer pointer to the allocated rsa_op
 * @limbs: number of allocated limbs (32 bit digits)
 *
 * The allocated rsa_op will be zeroed and not canonicalized
 */
int rsa_op_alloc(rsa_op **n, int limbs)
{
	rsa_op *handle;

	*n = NULL;
	if (!limbs)
		return -EINVAL;

	/* Allocate space for the rsa_op */
	handle = kmalloc(sizeof(rsa_op), GFP_KERNEL);
	if (!handle)
		return -ENOMEM;

	/* Allocate space for its data */
	handle->data = kzalloc(limbs * sizeof(u32), GFP_KERNEL);
	if (!handle->data) {
		kfree(handle);
		return -ENOMEM;
	}

	handle->sign = 0;
	handle->size = handle->limbs = limbs;
	*n = handle;
	return 0;
}

/**
 * rsa_op_free - free the resources held by an rsa_op
 * @n: pointer to the rsa_op to be freed
 */
void rsa_op_free(rsa_op *n)
{
	if (!n)
		return;
	kfree(n->data);
	kfree(n);
}

/**
 * rsa_op_init - initialize an rsa_op given its absolute value
 * @n: pointer pointer to the allocated rsa_op
 * @data: the value as a byte array
 * @size: sizeof(data)
 * @xtra: how many extra limbs to preallocate to avoid reallocations
 *
 * The optional leading zeroes will be taken into account, so that the
 * rsa_op created will not be canonicalized
 */
int rsa_op_init(rsa_op **n, u8 *data, u32 size, u32 xtra)
{
	int i, j, s, retval;
	u32 *buf;

	*n = NULL;
	if (!size && !xtra)
		return -EINVAL;

	/* Allocate space for the rsa_op and its data */
	s = (size + 3) / 4;
	retval = rsa_op_alloc(n, s + xtra);
	if (retval < 0)
		return retval;

	(*n)->size = s;
	buf = (*n)->data;

	/* Copy the data */
	for (i = size - 1, j = 0; i >= 0; i--, j++)
		buf[j / 4] |= ((u32)data[i] << ((j % 4) * 8));
	return 0;
}

/**
 * rsa_op_set - set the value of an rsa_op given its absolute value
 * @n: pointer pointer to the allocated rsa_op
 * @data: the value as a byte array
 * @size: sizeof(data)
 *
 * The optional leading zeroes will be taken into account, so that the rsa_op
 * created will not be canonicalized. The rsa_op passed in will be reallocated
 * (and relocated) if needed
 */
int rsa_op_set(rsa_op **n, u8 *data, u32 size)
{
	int s, i, j, retval;
	u32 *buf;

	if (!size)
		return -EINVAL;

	/* Size of the new rsa_op value (in limbs) */
	s = (size + 3) / 4;
	retval = rsa_op_resize(n, s);
	if (retval < 0)
		return retval;
	memset((*n)->data, 0, (*n)->size * sizeof(u32));

	/* Copy the data */
	buf = (*n)->data;
	for (i = size - 1, j = 0; i >= 0; i--, j++)
		buf[j / 4] |= ((u32)data[i] << ((j % 4) * 8));
	return 0;
}

/**
 * rsa_op_copy - rsa_op to rsa_op copy
 * @dest: destination rsa_op
 * @src: source rsa_op
 */
int rsa_op_copy(rsa_op **dest, rsa_op *src)
{
	int retval;

	retval = rsa_op_resize(dest, src->size);
	if (retval < 0)
		return retval;
	memcpy((*dest)->data, src->data, src->size * sizeof(u32));
	return 0;
}

/**
 * rsa_op_print - print the value of an rsa_op
 * @n: pointer to the rsa_op to be printed
 * @how: true to print canonicalized
 */
void rsa_op_print(rsa_op *n, bool how)
{
	int i, j;
	u32 limb;
	u8 byte;
	bool started = false;

	printk("Operand @ 0x%x, %d limbs in size, %d limbs allocated, value = ",
	       (u32)n, n->size, n->limbs);

	/* Print the sign */
	printk("%s", (n->sign)? "-": " ");

	/* Print the hex value */
	for (i = n->size - 1; i >= 0; i--) {
		limb = n->data[i];

		/* Ignore leading zero limbs if canonicalization is selected */
		if (!limb && !started && how)
			continue;

		/*
		 * Print each limb as though each of its nibbles was a character
		 * from the set '0' to '9' and 'a' to 'f'
		 */
		for (j = 28; j >= 0; j -= 4) {
			byte = (u8)((limb >> j) & 0x0F);

			/*
			 * Ignore leading zero bytes if canonicalization
			 * is selected
			 */
			if (!byte && !started && how)
				continue;

			started = true;
			byte += (byte <= 0x09)? '0': 'a' - 0x0A;
			printk("%c", byte);
		}
	}

	if(!started)
		printk("0");
	printk("\n");
}

/**
 * rsa_decode - decode the string value
 */
void rsa_decode (rsa_op *op, unsigned int digits, unsigned char *b, unsigned int len)
{
    u_long t;
    int j;
    unsigned int i, u;
    u_long *a = op->data;
    
    for (i = 0, j = len - 1; i < digits && j >= 0; i++) {
        t = 0;
        for (u = 0; j >= 0 && u < RSA_DIGIT_BITS; j--, u += 8)
        		t |= ((u_long)b[j]) << u;
        	a[i] = t;
    }

    for (; i < digits; i++)
        a[i] = 0;
}

/**
 * rsa_cipher - compute montgomery modular exponentiation, m^e mod n
 * @res: pointer pointer to the allocated result
 * @m: data to be ciphered, also base of the exponentiation
 * @e: exponent
 * @n: modulus
 *
 * The montgomery modular exponentiation is not generic. It can be used only
 * in cases where the modulus is odd as it is with RSA public and private key
 * pairs, and this is because the algorithm depends in such number properties
 * to speed things up. The calculation results will be invalid if the modulus
 * is even. Furthermore the base must be in the residue class of the modulus
 * (m < n). The calculation result if m > n will be mathematically valid but
 * will be useless for the RSA cryptosystem, because it will be the result of
 * (m mod n)^e mod n and not m^e mod n. Also the bit width of the base must
 * be equal to the modulus bit width.
 */
int rsa_cipher(rsa_op **res, rsa_op *m, rsa_op *e, rsa_op *n)
{
	int i, j, retval;
	u32 limb, modinv;
	bool started = false;
	rsa_op *aux[AUX_OPERAND_COUNT], *M, *x, *tmp;

#if 1
#define RSA_CIPHER_STEP(...) do{}while(0)
#else
#define RSA_CIPHER_STEP printk
#endif
	/*
	 * Check for bit width equality
	 * Check if the base is in the residue class of the modulus
	 * Check if the modulus is odd
	 */
	RSA_CIPHER_STEP("Step 1 m size %d, n size %d, n data[0] 0x%x %d\n", 
	                m->size, n->size, n->data[0], rsa_op_compare(m, n));
	
	if (m->size != n->size || rsa_op_compare(m, n) > 0 || !(n->data[0] & 1))
		return -EINVAL;

    RSA_CIPHER_STEP("Step 2\n");
	modinv = rsa_op_modinv32(n);
	memset(&aux, 0, sizeof(aux));
	M = x = NULL;

	for (i = 0; i < AUX_OPERAND_COUNT; i++) {
		retval = rsa_op_alloc(&aux[i],
				      CONFIG_RSA_AUX_OPERAND_SIZE);
		if (retval < 0)
			goto rollback;
	}
    RSA_CIPHER_STEP("Step 3\n");

	/* Compute M = m*r mod n where r is 2^k and k is the bit length of n */
	retval = rsa_op_copy(&M, m);
	if (retval < 0)
		goto rollback;
    RSA_CIPHER_STEP("Step 4\n");

	retval = rsa_op_shift(&M, -(n->size * 32));
	if (retval < 0)
		goto rollback;
    RSA_CIPHER_STEP("Step 5\n");
    RSA_DUMP("Cipher data: ", M->data, M->size);

	retval = rsa_remainder(&M, M, n, aux);
	if (retval < 0)
		goto rollback; 
    RSA_CIPHER_STEP("Step 6\n");
    RSA_DUMP("Cipher data: ", M->data, M->size);

	/* Compute x = r mod n where r is 2^k and k is the bit length of n */
	retval = rsa_op_init(&x, "\x01", 1, 32);
	if (retval < 0)
		goto rollback;
    RSA_CIPHER_STEP("Step 7\n");

	retval = rsa_op_shift(&x, -(n->size * 32));
	if (retval < 0)
		goto rollback;
    RSA_CIPHER_STEP("Step 8\n");

	retval = rsa_remainder(&x, x, n, aux);
	if (retval < 0)
		goto rollback;
    RSA_CIPHER_STEP("Step 9\n");

	/*
	 * Canonicalize the exponent and compute the modular exponentiation.
	 * For each limb of the exponent perform left to right binary
	 * exponentiation
	 */
	rsa_op_canonicalize(e);
	for (i = e->size - 1; i >= 0; i--) {
		/* Take a limb of the exponent */
		limb = e->data[i];

		/* For each of its bits */
		for (j = 0; j < 32; j++) {
			/*
			 * While the exponent has non significant zeroes shift
			 * it to the left
			 */
			if (!(limb & U32_MSB_SET) && !started) {
				limb = limb << 1;
				continue;
			}
			started = true;

			/* Compute x = x*x mod n */
			retval = rsa_mproduct(&x, x, x, n, modinv, aux);
			if (retval < 0)
				goto rollback;

			if (limb & U32_MSB_SET) {
				/* Compute x = M*x mod n */
				retval = rsa_mproduct(&x, x, M, n, modinv, aux);
				if (retval < 0)
					goto rollback;
			}

			limb = limb << 1;
		}
	}
    RSA_CIPHER_STEP("Step 10\n");
    RSA_DUMP("Cipher data: ", x->data, x->size);
    RSA_DUMP("Cipher data: ", M->data, M->size);

	/* Compute res = x mod n */
	retval = rsa_op_init(&tmp, "\x01", 1, 32);
	if (retval < 0)
		goto rollback;
    RSA_CIPHER_STEP("Step 11\n");
	retval = rsa_mproduct(res, x, tmp, n, modinv, aux);
    RSA_LOG("RSA Cipher, leave %d\n", retval);

/* Free all allocated resources */
rollback:
	for (i = 0; i < AUX_OPERAND_COUNT; i++)
		rsa_op_free(aux[i]);
	rsa_op_free(M);
	rsa_op_free(x);
	rsa_op_free(tmp);

#undef RSA_CIPHER_STEP
    
	return retval;
}

EXPORT_SYMBOL(rsa_op_alloc);
EXPORT_SYMBOL(rsa_op_free);
EXPORT_SYMBOL(rsa_op_init);
EXPORT_SYMBOL(rsa_op_set);
EXPORT_SYMBOL(rsa_op_copy);
EXPORT_SYMBOL(rsa_op_print);
EXPORT_SYMBOL(rsa_cipher);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tasos Parisinos @ Sciensis Advanced Technology Systems");
MODULE_DESCRIPTION("RSA cipher algorithm implementation");
MODULE_ALIAS("rsa");
