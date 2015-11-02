#include <sys/types.h> // работа с каталогами
#include <dirent.h>    // работа с каталогами
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unordered_set>

const char *GROUP_CATALOG_NAME = "GROUP_CATALOG.";

bool process_group(const char*, const char*);
int filter(const struct dirent*);

int filter(const struct dirent *entry)
{
  int ret = 0;

  if (0 == (strncmp(entry->d_name, GROUP_CATALOG_NAME, strlen(GROUP_CATALOG_NAME))))
    ret = 1;

  return ret;
}

// work_env - рабочий каталог
// file_name - название файла с описанием группы (e.x. GROUP_CATALOG.KD2022_SS)
bool process_group(const char* work_env, const char* file_name)
{
  bool stat = false;
  char group_file_name[200 + 1];
  char one_line[200+1];
  char point_name[200+1];
  std::unordered_set <std::string> sbs_points_set;
  const char *group_name = NULL;
  const char *last_point = NULL;
  FILE *group_file = NULL;

  // Вырезать из названия файла GROUP_CATALOG.KD2022_SS
  group_name = strrchr(file_name, '.');
  if (group_name)
  {
    // Пропустить символ '.'
    group_name++;
    // Собрать имя файла группы
    snprintf(group_file_name, 200, "%s/%s", work_env, group_name);
    // Удалось открыть файл группы?
    if (NULL != (group_file = fopen(group_file_name, "r")))
    {
      printf("Read group name: %s\n", group_name);
    
      while(fgets(one_line, 200, group_file))
      {
        last_point = strrchr(one_line, '.');
        if (last_point)
        {
          strncpy(point_name, one_line, last_point - one_line);
          point_name[last_point - one_line] = '\0';

          sbs_points_set.insert(point_name);
//1          printf("%02d) %s\t%s\n", last_point - one_line, one_line, point_name);
        }
      }

      fclose(group_file);

      // У данной Группы непустое множество Точек
      if (sbs_points_set.size())
      {
        for (const std::string& x: sbs_points_set)
        {
          printf("\t%s\n", x.c_str());
        }
      }

      stat = true;
    }
  }

  return stat;
}

int main(int argc, char *argv[])
{
  const char* work_env_def = "/home/gev/ITG/sandbox/rtdbus/test/dat/SINF";
  struct dirent **namelist;
  int n;
  bool stat = false;
  char work_env[200 + 1];

  if (argc == 2)
    strncpy(work_env, 200, argv[1]);
  else
    strcpy(work_env, work_env_def);
   
  printf("Workdir is '%s'\n", work_env);

  n = scandir(work_env, &namelist, filter, alphasort);
  if (n < 0)
  {
    perror("scandir");
  }
  else
  {
    while (n--)
    {
      printf("%s\n", namelist[n]->d_name);

      stat = process_group(work_env, namelist[n]->d_name);

      free(namelist[n]);
    }
    free(namelist);
  }


  return 0;
}

