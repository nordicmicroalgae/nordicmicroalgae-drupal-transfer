# Nordic Microalgae Drupal Transfer

This repository includes content and tools necessary to populate a new Drupal database with some initial content, such as basic information pages etc. The tools and content included in this repository were primarily used when moving CMS specific content (such as pages, news and forums) to a new Drupal database in March 2017, but may also be useful for setting up test and development environments that somewhat reflects the basic information structure and content of the production environment.

## Content

All content included in this repository were extracted from nordicmicroalgae.org on March 8 2017 and consists of YAML documents prepared with title, author, date and body for each article.

The following documents are available:

- `content/forums.yml`
- `content/forums_dev.yml`
- `content/news.yml`
- `content/news_dev.yml`
- `content/pages.yml`
- `content/pages_dev.yml`

File names ending with `_dev.yml` has references to nordicmicroalgae.org replaced by local.nordicmicroalgae.org, which may be useful for development/testing purposes.

## Tools

The following drush commands are available for Drupal 7:

* `nordicmicroalgae-import-forums`
* `nordicmicroalgae-import-news`
* `nordicmicroalgae-import-pages`
* `nordicmicroalgae-import-users`

Each command takes one argument, which is the file path to a YAML document.

Example command:

```
drush nordicmicroalgae-import-pages /path/to/content/pages.yml
```

The file `drush/nordicmicroalgae.d7.drush.inc` contains all the commands and should be installed in one of drush's detectable command directories (such as `~/.drush`). For more information on how to install and use drush please visit http://www.drush.org/

### Requirements

The following software must be installed in order to use the drush commands:

* [Drupal 7](https://www.drupal.org/)
* [Drush](http://www.drush.org/)
* [LibYAML](http://pyyaml.org/wiki/LibYAML)
* [YAML extension for PHP](http://php.net/manual/en/yaml.installation.php)
* [PHP Markdown](https://michelf.ca/projects/php-markdown/)
