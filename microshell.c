#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

# define FALSE	0
# define TRUE	1

# define STDIN	0
# define STDOUT	1
# define STDERR	2

# define SIDE_OUT	0
# define SIDE_IN	1

# define TYPE_END	0
# define TYPE_PIPE	1
# define TYPE_BREAK	2

typedef struct			s_list_cmds
{
	int			size;
	int			type;
	int			pipe[2];
	char			**args;
	struct s_list_cmds	*prev;
	struct s_list_cmds	*next;
}				t_list_cmds;

int		ft_strlen(char *str)
{
	int	i = 0;

	if (!str)
		return (0);
	while(str[i])
		i++;
	return (i);
}

int		ft_error(char *str)
{
	write(STDERR, str, ft_strlen(str));
	return (EXIT_FAILURE);
}

char		*ft_strdup(char *str)
{
	int	i;
	char	*new;

	if(!str)
		return (NULL);
	i = ft_strlen(str);
	new = (char*)malloc(sizeof(char) * (i + 1));
	if (!new)
		return (NULL);
	new[i] = 0;
	while(--i >= 0)
		new[i] = str[i];
	return (new);
}

int		check_end(char *end)
{
	if (!end)
		return (TYPE_END);
	if (strcmp(end, "|") == 0)
		return (TYPE_PIPE);
	if (strcmp(end, ";") == 0)
		return (TYPE_BREAK);
	return (0);
}

int		nb_arg(char **argv)
{
	int	size = 0;

	while(argv[size] && strcmp(argv[size], "|") && strcmp(argv[size],";"))
		size++;
	return (size);
}

void		ft_lstadd_back(t_list_cmds **cmds, t_list_cmds *new)
{
	t_list_cmds	*tmp;

	if(*cmds == NULL)
		*cmds = new;
	else
	{
		tmp = *cmds;
		while(tmp->next)
			tmp = tmp->next;
		tmp->next = new;
		new->prev = tmp;
	}
}

/*void		print_cmd(t_list_cmds *cmd)
{
	int	i = 0;

	printf("**********************\n");
	printf("CMD %s : ", cmd->args[0]);
	while(i < cmd->size)
		printf(" [%s] ", cmd->args[i++]);
	printf("\n\ttype = %d\tsize = %d\n\tPrev : ", cmd->type, cmd->size);
	if (cmd->prev)
		printf("%s", cmd->prev->args[0]);
	else
		printf("Null");
	printf("\n\tNext : ");
	if (cmd->next)
		printf("%s\n", cmd->next->args[0]);
	else
		printf("Null\n");
	printf("------------------------\n");
}*/

void		clear_cmds(t_list_cmds *cmds)
{
	int		i;
	t_list_cmds	*tmp;

	while(cmds)
	{
		tmp = cmds->next;
		i = 0;
		while(i < cmds->size)
			free(cmds->args[i++]);
		free(cmds->args);
		free(cmds);
		cmds = tmp;
	}
	cmds = NULL;
}

int		parse_arg(t_list_cmds **cmds, char **argv)
{
	t_list_cmds	*new;
	int		size;

	size = nb_arg(argv);
	new = (t_list_cmds *)malloc(sizeof(t_list_cmds));
	if(!new)
		exit(ft_error("error: fatal\n"));
	new->args = (char**)malloc(sizeof(char *) * (size + 1));
	if(!new->args)
		exit(ft_error("error: fatal\n"));
	new->size = size;
	new->args[size] = NULL;
	new->prev = NULL;
	new->next = NULL;
	while(--size >= 0)
		new->args[size] = ft_strdup(argv[size]);
	new->type = check_end(argv[new->size]);
	ft_lstadd_back(cmds, new);
	return (new->size);
}

void		exec_cmd(t_list_cmds *cmd, char **env)
{
	int	pipe_open = FALSE;
	pid_t	pid;

	if(cmd->type == TYPE_PIPE || (cmd->prev && cmd->prev->type == TYPE_PIPE))
	{
		pipe_open = TRUE;
		pipe(cmd->pipe);
	}
	pid = fork();
	if(pid < 0)
		exit(ft_error("error: fatal\n"));
	if (pid == 0)
	{
		if (cmd->type == TYPE_PIPE)
		{
			if(dup2(cmd->pipe[SIDE_IN], STDOUT) < 0)
				exit(ft_error("error: fatal\n"));
		}
		if (cmd->prev && cmd->prev->type == TYPE_PIPE)
		{
			if(dup2(cmd->prev->pipe[SIDE_OUT], STDIN) < 0)
				exit(ft_error("error: fatal\n"));
		}
		if (execve(cmd->args[0], cmd->args, env) < 0)
		{
			ft_error("error: cannot execute ");
			ft_error(cmd->args[0]);
			exit(ft_error("\n"));
		}
		exit(EXIT_SUCCESS);
	}
	else
	{
		int	status;
		waitpid(pid, &status, 0);
		if(pipe_open)
		{
			close(cmd->pipe[SIDE_IN]);
			if(cmd->next == NULL || cmd->type == TYPE_BREAK)
				close(cmd->pipe[SIDE_OUT]);
		}
		if (cmd->prev && cmd->prev->type == TYPE_PIPE)
			close(cmd->prev->pipe[SIDE_OUT]);
	}
}

void		run_cmds(t_list_cmds *cmds, char **env)
{
	t_list_cmds	*tmp;

	tmp = cmds;
	while(tmp)
	{
		if(strcmp(tmp->args[0], "cd") == 0)
		{
			if(tmp->size != 2)
			{
				ft_error("error: cd : bad arguments\n");
			}
			else if (chdir(tmp->args[1]) < 0)
			{
				ft_error("error: cd: cannot change dir to ");
				ft_error(tmp->args[1]);
				ft_error("\n");
			}
		}
		else
			exec_cmd(tmp, env);
		tmp = tmp->next;
	}
}

int		main(int argc, char **argv, char **env)
{
	int		i = 1;
	t_list_cmds	*cmds = NULL;

	if (argc > 1)
	{
		while(argv[i])
		{
			if(strcmp(argv[i], ";") == 0)
			{
				i++;
				continue;
			}
			i += parse_arg(&cmds, &argv[i]);
			if (argv[i] == NULL)
				break;
			else
				i++;
		}
		if(cmds)
			run_cmds(cmds, env);
		clear_cmds(cmds);
	}
	return (0);
}
