/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_flagcheck.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: estruckm <estruckm@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/02 15:08:17 by estruckm          #+#    #+#             */
/*   Updated: 2023/01/06 22:35:57 by estruckm         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static int	put_padded_hex(t_format f, va_list args)
{
	const char		*base;
	unsigned int	n;
	int				len;
	int				pad;

	base = (f.type == 'x') ? "0123456789abcdef" : "0123456789ABCDEF";
	n = va_arg(args, unsigned int);
	len = (int)ft_numlen_base(n, 16);
	pad = 0;
	while (f.zero && (len + pad) < f.width)
	{
		ft_putchar('0');
		pad++;
	}
	ft_puthex(n, base, 16);
	return (len + pad);
}

int	ft_flagcheck(t_format f, va_list args)
{
	if (f.type == 'd' || f.type == 'i')
		return (ft_putnbr_base(va_arg(args, int), "0123456789", 10));
	if (f.type == 'c')
		return (ft_putchar(va_arg(args, int)));
	if (f.type == 's')
		return (ft_putstr(va_arg(args, char *)));
	if (f.type == 'u')
		return (ft_putnbr_base(va_arg(args, unsigned int), "0123456789", 10));
	if (f.type == 'x' || f.type == 'X')
		return (put_padded_hex(f, args));
	if (f.type == 'p')
		return (ft_putpointer(va_arg(args, unsigned long long)));
	if (f.type == 'z')
		return (ft_putnbr_base(va_arg(args, size_t), "0123456789", 10));
	if (f.type == '%')
		return (ft_putchar('%'));
	return (0);
}

