/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ft_printf.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: estruckm <estruckm@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2023/01/02 14:49:44 by estruckm          #+#    #+#             */
/*   Updated: 2023/01/06 22:38:23 by estruckm         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ft_printf.h"

static void	parse_format(const char *s, int *i, t_format *f)
{
	f->zero = 0;
	f->width = 0;
	(*i)++;
	if (s[*i] == '0')
	{
		f->zero = 1;
		(*i)++;
	}
	while (s[*i] >= '1' && s[*i] <= '9')
	{
		f->width = f->width * 10 + (s[*i] - '0');
		(*i)++;
	}
	f->type = s[*i];
	if (f->type == 'z')
		(*i)++;
}

int	ft_printf(const char *s, ...)
{
	va_list		arguments;
	t_format	f;
	int			i;
	int			j;

	i = 0;
	j = 0;
	if (s[i] == '\0')
		return (0);
	va_start(arguments, s);
	while (s[i] != '\0')
	{
		if (s[i] == '%')
		{
			parse_format(s, &i, &f);
			j += ft_flagcheck(f, arguments);
		}
		else
			j += ft_putchar(s[i]);
		i++;
	}
	va_end(arguments);
	return (j);
}
