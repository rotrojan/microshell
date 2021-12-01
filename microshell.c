#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

typedef enum e_bool
{
	False,
	True
}	t_bool;

typedef struct s_list
{
	char			**argv;
	int				fd_in;
	int				fd_out;
	struct s_list	*next;
}	t_list;


int	ft_strlen(char const *str)
{
	int i = 0;

	while (str[i] != '\0')
		++i;
	return (i);
}

void	ft_putstr_fd(char const *str, int fd)
{
	write(fd, str, ft_strlen(str));
}

int	print_error(char const *str, char const *arg)
{

	ft_putstr_fd("error: ", STDERR_FILENO);
	ft_putstr_fd(str, STDERR_FILENO);
	if (arg != NULL)
		ft_putstr_fd(arg, STDERR_FILENO);
	ft_putstr_fd("\n", STDERR_FILENO);
	return (EXIT_FAILURE);
}

t_list	*new_link(void)
{
	t_list	*list;

	if ((list = malloc(sizeof(*list))) == NULL)
	{
		print_error("fatal", NULL);
		return (NULL);
	}
	list->argv = NULL;
	list->fd_in = STDIN_FILENO;
	list->fd_out = STDOUT_FILENO;
	list->next = NULL;
	return (list);
}

t_bool	add_args(char ***list_argv, char *arg)
{
	int		size_array = 0;
	int		i = 0;
	char	**new_array = NULL;

	if (*list_argv != NULL)
		while ((*list_argv)[size_array] != NULL)
			size_array++;
	if ((new_array = malloc(sizeof(*new_array) * (size_array + 2))) == NULL)
	{
		print_error("fatal", NULL);
		return (False);
	}
	while (*list_argv != NULL && (*list_argv)[i] != NULL)
	{
		new_array[i] = (*list_argv)[i];
		i++;
	}
	new_array[i] = arg;
	new_array[++i] = NULL;
	free(*list_argv);
	*list_argv = new_array;
	return (True);
}

t_list	*build_cmd(int *i, char **argv)
{
	t_list	*link = NULL;

	if ((link = new_link()) == NULL)
		return (NULL);
	while (argv[*i] != NULL
			&& strcmp(argv[*i], "|") != 0
			&& strcmp(argv[*i], ";") != 0)
	{
		if (add_args(&link->argv, argv[*i]) == False)
			return (NULL);
		(*i)++;
	}
	return (link);
}

void	lst_add_back(t_list *new, t_list **lst)
{
	t_list	*current;

	if (*lst == NULL)
		*lst = new;
	else
	{
		current = *lst;
		while (current->next != NULL)
			current = current->next;
		current->next = new;
	}
}

void	free_lst(t_list **lst)
{
	t_list	*current;
	t_list	*tmp;

	current = *lst;
	while (current != NULL)
	{
		tmp = current->next;
		free(current->argv);
		current->argv = NULL;
		free(current);
		current = NULL;
		current = tmp;
	}
	*lst = NULL;
}

t_list	*build_lst(int *i, char **argv)
{
	t_list	*lst = NULL;
	t_list	*new = NULL;

	while (argv[*i] != NULL && strcmp(argv[*i], ";") != 0)
	{
		new = build_cmd(i, argv);
		if (new == NULL)
		{
			free_lst(&lst);
			return (NULL);
		}
		lst_add_back(new, &lst);
		if (argv[*i] != NULL && strcmp(argv[*i], "|") == 0)
			++(*i);
	}
	return (lst);
}

void	cd(char **argv)
{
	if (argv[1] == NULL || argv[2] != NULL)
		print_error("cd: bad arguments", NULL);
	else
		if (chdir(argv[1]) == -1)
			print_error("cd: cannot change directory to ", argv[1]);
}

void	exec_lst(t_list *lst, char **envp)
{
	pid_t	pid;
	int		fd[2];

	while (lst != NULL)
	{
		if (strcmp(lst->argv[0], "cd") == 0)
			cd(lst->argv);
		else
		{
			if (lst->next != NULL)
			{
				pipe(fd);
				lst->fd_out = fd[1];
				lst->next->fd_in = fd[0];
			}
			pid = fork();
			if (pid == -1)
			{
				print_error("fatal", NULL);
				return ;
			}
			if (pid != 0)
			{
				waitpid(pid, NULL, 0);
				if (lst->fd_in != STDIN_FILENO)
					close(lst->fd_in);
				if (lst->fd_out != STDOUT_FILENO)
					close(lst->fd_out);
			}
			else
			{
				dup2(lst->fd_in, STDIN_FILENO);
				dup2(lst->fd_out, STDOUT_FILENO);
				if (execve(lst->argv[0], lst->argv, envp) == -1)
					print_error("cannot execute ", lst->argv[0]);
			}
		}
		lst = lst->next;
	}
}

int	main(int ac, char **av, char **envp)
{
	t_list	*lst = NULL;
	int		i = 1;

	if (ac == 1)
		return (EXIT_SUCCESS);
	while (av[i] != NULL)
	{
		while (av[i] != NULL && strcmp(av[i], ";") == 0)
			++i;
		if (av[i] == NULL)
			break ;
		lst = build_lst(&i, av);
		if (lst == NULL)
			return (EXIT_FAILURE);
		exec_lst(lst, envp);
		free_lst(&lst);
	}
	return (EXIT_SUCCESS);
}
