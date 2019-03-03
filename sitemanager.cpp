#include <iostream>
#include <iomanip>
#include <fstream>
#include <regex>
#include <string>
#include <sstream>
#include <vector>
#include <dirent.h>

#if defined(_WIN32)
#include <direct.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <sys/stat.h>
#endif

unsigned INDENT_AMOUNT = 4;
std::string siteroot("/var/www/www.zsmb.co/");
std::string sourceroot("/home/pi/sitemanager/sources/");

void makedir(std::string const& string) {
    const char* str = string .c_str();
#ifdef _WIN32
    mkdir(str);
#endif
#if defined(__linux__) || defined(__APPLE__)
    mkdir(str, 775);
#endif
}

std::istream& get_line(std::istream&  is, std::string& str) {
    std::istream& ret = getline(is, str);
    
    if(str != "" && str.back() == '\r') // fixes silly \r\n problems on linux
        str.pop_back();                 // string not empty check needed because calling back
    return ret;                         // on empty string causes undefined behaviour
}

void prepare_templates() {
    std::ifstream in(sourceroot+"template.html");
    std::string line;
    makedir("temp");
    
    constexpr int splits = 6;
    std::ofstream out[splits];
    
    for(int i = 0; i < splits; i++) {
        std::stringstream filename;
        filename << "temp/template_" << i << ".html";
        out[i].open(filename.str());
    }
    
    for(int i = 0; i < splits-1; i++) {
        while(get_line(in, line) && line[0] != '*') {
            out[i] << line << std::endl;
        }
    }
    
    while(get_line(in, line)) {
        out[splits-1] << line << std::endl;
    }
}

void print_template(std::ostream& out, unsigned n) {
    std::stringstream filename;
    filename << "temp/template_" << n << ".html";
    std::ifstream in(filename.str());
    std::string line;
    while(get_line(in, line)) {
        out << line << std::endl;
    }
}

std::vector<std::string> get_source_files(std::string folder) {
    std::vector<std::string> filenames;
    folder = sourceroot + folder;
    
    DIR* dir;
    struct dirent* ent;
    dir = opendir(folder.c_str());
    while((ent = readdir(dir)) != NULL) {
        if(std::regex_match(std::string(ent->d_name), std::regex(".*\\.txt$"))) {
            filenames.push_back(ent->d_name);
        }
    }
    closedir(dir);
    
    return filenames;
}

std::string clear_html(std::string str) {
    std::regex html_tag("^(.*)\\<.+?\\>(.*)$");
    std::regex double_space("^(.* ) (.*)$");
    std::smatch mat;
    
    while(regex_match(str, mat, html_tag)) {
        auto i = ++mat.begin();
        std::string beginning = *(i++);
        std::string end = *i;
        str = beginning + end;
    }
    
    while(regex_match(str, mat, double_space)) {
        auto i = ++mat.begin();
        std::string beginning = *(i++);
        std::string end = *i;
        str = beginning + end;
    }
    
    return str;
}

std::string& strip(std::string& str) {
    if(str == "")
        return str;
        
    while(str.front() == ' ')
        str = str.substr(1);
        
    while(str.back() == ' ')
        str.pop_back();
        
    return str;
}

std::string make_hyphenated(std::string s) {
    std::string temp;
    strip(s);
    
    for(unsigned i = 0; i < s.size(); i++) {
        if(('a' <= s[i] && s[i] <= 'z') || ('0' <= s[i] && s[i] <= '9')) {
            temp += s[i];
        }
        else if('A' <= s[i] && s[i] <= 'Z') {
            temp += (s[i]-'A'+'a');
        }
        else if(s[i] == ' ' || s[i] == '-' || s[i] == '_') {
            temp += '-';
        }
    }
    
    return temp;
}

std::string make_simple(std::string s) {
    std::string temp;
    strip(s);
    
    for(unsigned i = 0; i < s.size(); i++) {
        if(('a' <= s[i] && s[i] <= 'z') || ('0' <= s[i] && s[i] <= '9')) {
            temp += s[i];
        }
        else if('A' <= s[i] && s[i] <= 'Z') {
            temp += (s[i]-'A'+'a');
        }
    }
    
    return temp;
}

std::string make_lowercase(std::string s) {
    strip(s);
    
    for(unsigned i = 0; i < s.size(); i++) {
        if('A' <= s[i] && s[i] <= 'Z') {
            s[i] = s[i] - 'A' + 'a';
        }
    }
    
    return s;
}

std::string indent(unsigned extra = 0) {
    std::string s;
    for(unsigned i = 1; i <= INDENT_AMOUNT + extra; i++) {
        s += "    ";
    }
    return s;
}

std::string process(std::string str) {
    std::smatch mat;
    
    strip(str);
    
    std::regex link("^(.*?)\\[(.*?)\\]\\((.*?[^\\\\])\\)(?:\\{(.*?)\\})?(?:\"(.*?)\")?(.*)$");
    while(regex_match(str, mat, link)) {
        auto i = ++mat.begin();
        
        std::string beg = *(i++);
        
        std::string text = *(i++);
        std::string url = *(i++);
        std::string icon = *(i++);
        std::string hover = *(i++);
        
        std::string end = *(i++);
        
        bool external = (url[0] != '/');
        
        // process URL - replace \) with )
        // reuses mat variable because lazy!
        while(regex_match(url, mat, std::regex("^(.*?)\\\\\\)(.*)$"))) {
            auto i = mat.begin();
            i++;
            std::string b = *i;
            i++;
            std::string e = *i;
            
            url = b + ")" + e;
        }
        
        if(icon == "") { // no custom icon
            if(external) // external link
                icon = "fa-external-link";
            else // site link
                icon = "fa-link";
        }
        
        unsigned j= 0;
        std::string text_firstword;
        std::string text_rest;
        while(text[j] != ' ' && j < text.size()) {
            text_firstword += text[j];
            j++;
        }
        while(j < text.size()) {
            text_rest += text[j];
            j++;
        }
        
        str = beg + "<a href=\"" + url + '"' + ((hover!="")?(std::string(" title=\"") + hover + '"'):"")
              + (external?" target=\"_blank\"":"") + "><span class=\"nowrap\"><i class=\"fa " + icon + "\"></i>&nbsp;" + text_firstword + "</span>" + text_rest + "</a>" + end;
    }
    
    // at this point there are no links left so this regex is okay
    std::regex icon("^(.*?)\\{(.*?)\\}(.*)$");
    while(regex_match(str, mat, icon)) {
        auto i = ++mat.begin();
        std::string beg = *(i++);
        std::string name = *(i++);
        std::string end = *(i++);
        
        str = beg + "<i class=\"fa " + name + "\"></i>" + end;
    }
    
    return str;
}

class Content {
public:
    virtual std::string get() const = 0;
    virtual ~Content() {}
};

std::ostream& operator<<(std::ostream& os, Content const& c) {
    os << indent() << c.get();
    return os;
}

class Header : public Content {
    std::string text;
    unsigned short size;
public:
    Header(std::string header_text, unsigned short size = 3) : size(size) {
        text = process(header_text);
    }
    
    virtual std::string get() const {
        std::stringstream temp;
        temp << "<h" << size << '>' << text << "</h" << size << '>';
        return temp.str();
    }
};

class Paragraph : public Content {
    std::string text;
public:
    Paragraph(std::string paragraph_text) {
        text = process(paragraph_text);
    }
    
    virtual std::string get() const {
        return std::string("<p>") + text + "</p>";
    }
};

class Image : public Content {
    std::string path;
    std::string alt_text;
    bool large;
public:
    Image(std::string image_data, bool is_large = false) : large(is_large) {
        std::smatch mat;
        regex_match(image_data, mat, std::regex("(.*?) \\\"(.*)\\\""));
        auto i = mat.begin();
        path = *(++i);
        alt_text = *(++i);
    }
    
    virtual std::string get() const {
        std::stringstream temp;
        temp << "<img ";
        if(large)
            temp << "class=\"articleimage\" ";
        temp << "alt=\"" << alt_text << "\" "
             << "src=\"" << path << "\">";
        return temp.str();
    }
};

class Comment : public Content {
    std::string text;
public:
    Comment(std::string comment_text) : text(comment_text) {}
    
    virtual std::string get() const {
        return std::string("<!-- ") + text + " -->";
    }
};

class Code : public Content {
    std::string code;
    std::string lang;
public:
    Code(std::string code_contents, std::string language = "c++")
        : code(code_contents), lang(language) {}
        
    virtual std::string get() const {
        std::stringstream temp;
        temp << "<pre><code class=\"" << lang << "\">" << std::endl;
        temp << code;
        temp << indent() << "</code></pre>";
        return temp.str();
    }
};

class List : public Content {
    std::vector<std::string> items;
    bool ordered;
public:
    List(std::vector<std::string>& list_items, bool ordered = false)
        : items(list_items), ordered(ordered) {
        for(auto &s : items) {
            s = process(s);
        }
    }
    
    virtual std::string get() const {
        std::stringstream temp;
        temp << (ordered?"<ol>":"<ul>") << std::endl;
        for(auto s : items) {
            temp << indent(1) << "<li>" << s << "</li>" << std::endl;
        }
        temp << indent() << (ordered?"</ol>":"</ul>");
        return temp.str();
    }
};

struct DownloadItem {
    std::string name;
    std::string desc;
    std::string size;
    std::string date;
};

class DownloadTable : public Content {
    std::vector<DownloadItem> items;
    std::string* relativepath;
public:
    DownloadTable(std::vector<DownloadItem>& download_items, std::string* relativesitepath)
        : items(download_items), relativepath(relativesitepath) {}
        
    virtual std::string get() const {
        std::stringstream temp;
        temp << "<table class=\"dltable\">" << std::endl
             << indent(1) << "<tr>" << std::endl
             << indent(2) << "<th class=\"dlcolumn\"></th>" << std::endl
             << indent(2) << "<th>Name</th>" << std::endl
             << indent(2) << "<th>Description</th>" << std::endl
             << indent(2) << "<th>Size</th>" << std::endl
             << indent(2) << "<th>Date</th>" << std::endl
             << indent(1) << "</tr>" << std::endl;
             
        for(auto d : items) {
            temp << indent(1) << "<tr>" << std::endl
                 << indent(2) << "<td class=\"dlcolumn\"><a href=\"" << *relativepath << d.name << "\"><i class=\"fa fa-download\"></i></a></td>" << std::endl
                 << indent(2) << "<td>" << d.name << "</td>" << std::endl
                 << indent(2) << "<td>" << d.desc << "</td>" << std::endl
                 << indent(2) << "<td>" << d.size << "</td>" << std::endl
                 << indent(2) << "<td>" << d.date << "</td>" << std::endl
                 << indent(1) << "</tr>" << std::endl;
        }
        
        temp << indent() << "</table>";
        return temp.str();
    }
};

class CustomHTML : public Content {
    std::string text;
public:
    CustomHTML(std::string html) : text(html) {}
    
    virtual std::string get() const {
        return text;
    }
};

class Section {
    Header header;
    std::string id;
    std::string classname;
    std::vector<Content*> contents;
public:
    Section(Header h, std::string id, std::string classname = "") : header(h), id(id), classname(classname) {}
    
    void add(Content* c) {
        contents.push_back(c);
    }
    
    void destroy() {
        for(Content* c : contents) {
            delete c;
        }
    }
    
    friend std::ostream& operator<<(std::ostream& os, Section const& s); // TODO rethink this
};

std::ostream& operator<<(std::ostream& os, Section const& s) {
    os << indent() << "<section id=\"" << s.id << '"' << ((s.classname!="")?(std::string(" class=\"")+s.classname+"\""):"") << '>' << std::endl;
    INDENT_AMOUNT++;
    os << s.header << std::endl;
    for(auto c : s.contents) {
        os << *c << std::endl;
    }
    INDENT_AMOUNT--;
    os << indent() << "</section>" << std::endl;
    return os;
}

class Page;
std::vector<Page*>* site_posts; // sorcery

class Page {
protected:
    // all data is stored in here
    std::vector<Section> sections;
    // page properties
    unsigned short date[3];
    bool contains_code;
    bool has_banner;
    // subclass dependent properties
    
    void print(std::ostream& os);
public:
    // page properties
    std::string description;
    std::string title;
    std::string posttitle;
    std::string shorttitle;
    std::string lang;
    // subclass dependent properties
    std::string filename;
    std::string relativesitepath;
    std::vector<std::string> categories;
    std::string url;
    
    bool operator>(Page const& rhs) const {
        for(int i = 0; i < 3; i++) {
            if(date[i] > rhs.date[i]) return true;
            else if(date[i] < rhs.date[i]) return false;
        }
        return false;
    }
    
    std::string get_date() {
        std::stringstream temp;
        temp  << std::setfill('0') << std::setw(4) << date[0] << '.'
              << std::setfill('0') << std::setw(2) << date[1] << '.'
              << std::setfill('0') << std::setw(2) << date[2] << '.';
        return temp.str();
    }
    
    void title_debug() {
        std::cout << "title:" << title << std::endl;
        std::cout << "shorttitle:" << shorttitle << std::endl;
        std::cout << "posttitle:" << posttitle << std::endl;
        std::cout << "filename:" << filename << std::endl;
        std::cout << "relativesitepath:" << relativesitepath << std::endl;
        std::cout << "url:" << url << std::endl;
        std::cout << std::endl;
    }
    
    Page(std::string filename = "")
        : contains_code(false)
        , has_banner(false) {
        if(filename != "") {
            std::ifstream in(filename);
            std::string str;
            std::smatch mat;
            std::regex ol_exp("^\\d+\\. ?(.*)$");
            std::regex pageprop_exp("^([a-z_]+)\\=\\\"(.*)\\\"$");
            bool next_is_description = false;
            
            get_line(in, str);
            while(regex_match(str, mat, pageprop_exp)) {
                auto i = ++mat.begin();
                std::string prop_name = *(i++);
                std::string prop_value = *(i++);
                
                if(prop_name == "title") {
                    title = prop_value;
                }
                else if(prop_name == "shorttitle") {
                    shorttitle = prop_value;
                }
                else if(prop_name == "date") {
                    std::stringstream temp(prop_value);
                    for(int i = 0; i < 3; i++) {
                        temp >> date[i];
                    }
                }
                else if(prop_name == "category") {
                    std::string temp;
                    std::stringstream value(prop_value);
                    while(value >> temp) {
                        if(temp == "Project") {
                            temp = "New project";
                        }
                        categories.push_back(temp);
                    }
                }
                else if(prop_name == "post") {
                    if(prop_value == "true") {
                        site_posts->push_back(this);
                    }
                }
                else if(prop_name == "lang") {
                    std::string languages[] = {"c", "cpp", "java"};
                    for(std::string l : languages) {
                        if(prop_value == l) {
                            lang = prop_value;
                            break;
                        }
                    }
                }
                else {
                    std::cerr << "unknown prop" << std::endl;
                }
                
                get_line(in, str);
            }
            
            std::sort(categories.begin(), categories.end());
            
            goto skip;
            
            while(get_line(in, str)) {
skip:
                if(str == "") { // empty line
                    // do nothing
                }
                else if(str[0] == '#' || str[0] == '@') { // header
                    Section temp(Header(str.substr((str[1]=='#')?2:1), (str[0]=='@')?2:3),
                                 std::string("section_") + std::to_string(sections.size()));
                    sections.push_back(temp);
                    
                    if(str[0] == '@' || (str[1] == '#' && description == "")) {
                        next_is_description = true;
                    }
                }
                else if(str[0] == ']') { // small image
                    sections.back().add(new Image(str.substr(1)));
                }
                else if(str[0] == '[') { // large image
                    sections.back().add(new Image(str.substr(1), true));
                }
                else if(str == "cpp{") { // cpp code
                    contains_code = true;
                    
                    std::stringstream temp;
                    get_line(in, str);
                    while(str != "}cpp") {
                        temp << str << std::endl;
                        get_line(in, str);
                    }
                    sections.back().add(new Code(temp.str(), "c++"));
                }
                else if(str == "%") { // download table TODO only for projects?
                    std::vector<DownloadItem> temp;
                    
                    while(str == "%") {
                        DownloadItem d;
                        get_line(in,d.name);
                        get_line(in,d.desc);
                        get_line(in,d.size);
                        get_line(in,d.date);
                        temp.push_back(d);
                        
                        get_line(in,str);
                    }
                    
                    sections.back().add(new DownloadTable(temp, &url));
                    goto skip;
                }
                else if(str[0] == '-') { // unordered list
                    std::vector<std::string> temp;
                    
                    while(str[0] == '-') {
                        temp.push_back(str.substr(1));
                        get_line(in, str);
                    }
                    
                    sections.back().add(new List(temp, false));
                    goto skip;
                }
                else if(regex_match(str, ol_exp)) { // ordered list
                    std::vector<std::string> temp;
                    
                    while(regex_match(str, mat, ol_exp)) {
                        temp.push_back(*(++mat.begin()));
                        get_line(in, str);
                    }
                    
                    sections.back().add(new List(temp, true));
                    goto skip;
                }
                else if(str[0] == '/' && str[1] == '/') { // comment
                    //sections.back().add(new Comment(str.substr(2))); // decided to throw away comments instead
                }
                else { // nothing special, paragraph
                    sections.back().add(new Paragraph(str));
                    
                    if(next_is_description) {
                        description = process(str);
                        next_is_description = false;
                    }
                }
            }
        }
        else { // not generated
            // yolo TODO
        }
    }
    
    ~Page() {
        for(auto s : sections) {
            s.destroy();
        }
    }
};

void Page::print(std::ostream& os) {
    print_template(os, 0);
    
    os << "    <meta name=\"description\" content=\"" << clear_html(description) << "\">" << std::endl;
    
    print_template(os, 1);
    
    if(contains_code) {
        os << "    <link rel=\"stylesheet\" href=\"/resources/darkula.css\">" << std::endl;
    }
    
    print_template(os, 2);
    
    if(contains_code) {
        os << indent() << "<script src=\"/resources/highlight.js\"></script>" << std::endl
           << indent() << "<script>hljs.initHighlightingOnLoad();</script>" << std::endl;
    }
    
    if(has_banner) {
        os << indent() << "<section id=\"content_header\">" << std::endl
           << indent(1) << "<h2 id=\"title\">" << title << "</h2>" << std::endl
           << indent(1) << "<h5 id=\"date\"><i class=\"fa fa-calendar-o\"></i>&nbsp;" << std::endl
           << indent(1) << get_date() << std::endl
           << indent(1) << "</h5>" << std::endl
           << indent(1) << "<img id=\"banner\" src=\"/images/" << make_simple(shorttitle) << "_ban.png\" alt=\""
           << title << "\">" << std::endl
           << indent() << "</section>" << std::endl;
    }
    
    for(auto s : sections) {
        os << s;
    }
    
    print_template(os, 3);
    print_template(os, 4);
    print_template(os, 5);
}

class Article : public Page {
public:
    Article(std::string filename) : Page(filename) {
        has_banner = true;
        posttitle = title;
        relativesitepath = "/articles/";
        
        std::stringstream temp;
        // old article naming with date in title
        /*temp << std::setfill('0') << std::setw(4) << date[0] << '-'
             << std::setfill('0') << std::setw(2) << date[1] << '-'
             << std::setfill('0') << std::setw(2) << date[2] << '-'
             << make_hyphenated(shorttitle);*/
        temp << make_hyphenated(shorttitle);
        Page::url = relativesitepath + temp.str() + '/';
        temp << ".html";
        Page::filename = temp.str();
        
    }
    
    void print() {
        std::ofstream out(siteroot+relativesitepath+filename);
        Page::print(out);
    }
};

class Project : public Page {
public:
    Project(std::string file_name) : Page(file_name) {
        has_banner = true;
        posttitle = "New project: ";
        posttitle += title;
        Page::filename = make_simple(shorttitle) + ".html";
        relativesitepath = std::string("/projects/") + make_simple(shorttitle) + "/";
        url = relativesitepath;
        categories.push_back("New project");
    }
    
    bool operator>(Project const& rhs) const {
        for(int i = 0; i < 3; i++) {
            if(date[i] > rhs.date[i]) return true;
            else if(date[i] < rhs.date[i]) return false;
        }
        return false;
    }
    
    void print() {
        std::ofstream out(siteroot+relativesitepath+filename);
        Page::print(out);
    }
};

class CustomPage : public Page {
public:
    CustomPage(std::string filename = "") : Page(filename) {
        contains_code = false;
        std::string relativesitepath = "/";
    }
    
    virtual void print(std::string filename = "") {
        if(filename == "") {
            std::cerr << "CUSTOM PAGE FILENAME ERROR" << std::endl;
        }
        else {
            std::ofstream os(siteroot+filename);
            Page::print(os);
        }
    }
    
    virtual ~CustomPage() {}
};

class ProjectsPage : public CustomPage {
public:
    ProjectsPage(std::vector<Project*>& c_projects, std::vector<Project*>& a_projects) {
        description = "The collection of my finished and (sadly) abandoned projects.";
        
        sections.push_back(Section(Header("Completed projects", 2), "completed", "projectsection"));
        sections.back().add(new Paragraph("These are my C/C++ programming projects that I've deemed finished. I've included the source code for the ones that I've managed to keep presentable."));
        
        sections.back().add(new CustomHTML("<div class=\"boxcontainer\">"));
        for(auto p : c_projects) {
            std::stringstream projecthtml;
            projecthtml << "<div class=\"projectbox\">" << std::endl
                        << indent(2) << "<a href=\"" << p->url << "\">" << std::endl;
            if(p->lang != "") {
                projecthtml  << indent(2) << "<img class=\"projectlang\" src=\"/images/lang_" << p->lang << ".png\">" << std::endl; 
            }
            projecthtml << indent(2) << "<img class=\"projectbg\" src=\"/images/" << make_simple(p->shorttitle) << "_pr.png\" alt=\"" << p->title << "\">" << std::endl
                        << indent(2) << "<img class=\"projectimage\" src=\"/images/" << make_simple(p->shorttitle) << "_pr.png\" alt=\"" << p->title << "\">" << std::endl
                        << indent(2) << "<h4>" << make_lowercase(p->title) << "</h4>" << std::endl
                        << indent(2) << "</a>" << std::endl
                        << indent(1) << "</div>";
            sections.back().add(new CustomHTML(projecthtml.str()));
        }
        sections.back().add(new CustomHTML("</div>"));
        
        sections.push_back(Section(Header("Abandoned projects", 2), "abandoned", "projectsection"));
        sections.back().add(new Paragraph("These are the projects that I gave up on, at least for now. They either deemed to be too complicated or I just lost interest in developing them further."));
        
        sections.back().add(new CustomHTML("<div class=\"boxcontainer\">"));
        for(auto p : a_projects) {
            std::stringstream projecthtml;
            projecthtml << "<div class=\"projectbox\">" << std::endl
                        << indent(2) << "<a href=\"" << p->url << "\">" << std::endl;
            if(p->lang != "") {
                projecthtml  << indent(2) << "<img class=\"projectlang\" src=\"/images/lang_" << p->lang << ".png\">" << std::endl; 
            }
            projecthtml << indent(2) << "<img class=\"projectbg\" src=\"/images/" << make_simple(p->shorttitle) << "_pr.png\" alt=\"" << p->title << "\">" << std::endl
                        << indent(2) << "<img class=\"projectimage\" src=\"/images/" << make_simple(p->shorttitle) << "_pr.png\" alt=\"" << p->title << "\">" << std::endl
                        << indent(2) << "<h4>" << make_lowercase(p->title) << "</h4>" << std::endl
                        << indent(2) << "</a>" << std::endl
                        << indent(1) << "</div>";
            sections.back().add(new CustomHTML(projecthtml.str()));
        }
        sections.back().add(new CustomHTML("</div>"));
    }
    
    void print(std::string filename = "") {
        CustomPage::print("projects.html");
    }
};

class ArchivesPage : public CustomPage {
public:
    ArchivesPage(std::vector<Page*>& posts) {
        description = "This page is a list of all posts that have been (or still are) on the Blog page of my site.";
        
        sections.push_back(Section(Header("Archives", 2), "archives"));
        sections.back().add(new Paragraph("This page is a list of all the posts from the Blog page of my site, sorted by date, with the most recent on top."));
        
        std::stringstream table_head;
        table_head << "<table>" << std::endl
                   << indent(1) << "<tr>" << std::endl
                   << indent(2) << "<th>Title</th>" << std::endl
                   << indent(2) << "<th>Category</th>" << std::endl
                   << indent(2) << "<th>Date</th>" << std::endl
                   << indent(1) << "</tr>";
        sections.back().add(new CustomHTML(table_head.str()));
        
        for(auto p : posts) {
            std::stringstream table_head;
            table_head << "<tr>" << std::endl
                       << indent(2) << "<td class=\"articletable\"><a href=\"" << p->url << "\">" << p->posttitle << "</a></td>" << std::endl
                       << indent(2) << "<td>";
                       
            // debug
            /*for(auto i : p->categories) {
                std::cout << i << std::endl;
            }*/
            
            for(int i = 0; i < int(p->categories.size())-1; i++) {
                table_head << p->categories[i] << " / ";
            }
            if(!p->categories.empty()) {
                table_head << p->categories[p->categories.size()-1];
            }
            
            table_head << "</td>" << std::endl
                       << indent(2) <<"<td>" << p->get_date() << "</td>" << std::endl
                       << indent(1) << "</tr>";
            sections.back().add(new CustomHTML(table_head.str()));
        }
        
        sections.back().add(new CustomHTML("</table>"));
        
        sections.push_back(Section(Header("Kittens", 2), "kittens"));
        sections.back().add(new Paragraph("There used to be kittens here while I didn't know what to do with this page, and there's no reason why they shouldn't be here anymore as far as I'm concerned."));
        sections.back().add(new Image("/images/kittens.jpg \"Kittens\"", true));
    }
    
    void print(std::string filename = "") {
        CustomPage::print("archives.html");
    }
};

class AboutPage : public CustomPage {
public:
    AboutPage() : CustomPage(sourceroot + "about.txt") {
    }
    
    void print(std::string filename = "") {
        CustomPage::print("about.html");
    }
};

class LostPage : public CustomPage {
public:
    LostPage() {
        description = "This is the page you get when you get lost on my site. I'm sorry.";
        
        sections.push_back(Section(Header("Uh-oh.", 2), "uhoh"));
        sections.back().add(new Paragraph("Hey there, stranger. You look lost. Use the menu above to find your way back."));
    }
    
    void print(std::string filename = "") {
        // this is custom even compared to the other CustomPages
        std::ofstream os(siteroot+"lost.html");
        
        print_template(os, 0);
        os << "    <meta name=\"description\" content=\"" << clear_html(description) << "\">" << std::endl;
        print_template(os, 1);
        print_template(os, 2);
        for(auto s : sections) {
            os << s;
        }
        print_template(os, 3);
        print_template(os, 5);
    }
};

class IndexPage : public CustomPage {
    unsigned page_num;
    unsigned last_page;
    
    std::string previous_page() {
        if(page_num < 2) {
            return "/";
        }
        
        std::stringstream temp;
        temp << "/page/" << page_num << '/';
        return temp.str();
    }
    
    std::string next_page() {
        if(page_num >= last_page) {
            return "/";
        }
        
        std::stringstream temp;
        temp << "/page/" << page_num+2 << '/';
        return temp.str();
    }
public:
    IndexPage(std::vector<Page*>& posts, unsigned page_number) : page_num(page_number) {
        description = "I'm a computer science student and this site is a place for my programming projects (mostly C and C++), and occasionally some posts about other things I'm interested in.";
        
        unsigned i = 10*page_number;
        unsigned limit = i + 10;
        last_page = posts.size()/10;
        if(last_page*10 < posts.size())
            last_page++;
            
        while(i < posts.size() && i < limit) {
            std::stringstream headertext;
            headertext << "<a href=\"" << posts[i]->url << "\">" << std::endl
                       << indent(2) << posts[i]->posttitle << std::endl << indent(1) << "</a>";
            sections.push_back(Section(Header(headertext.str(), 2), posts[i]->filename, "index")); // TODO CHANGE THIS FILENAME ISN'T WORKING WELL
            
            std::stringstream dateheader;
            dateheader << "<h5 class=\"date\"><i class=\"fa fa-calendar-o\"></i>&nbsp;" << std::endl
                       << indent(2) << posts[i]->get_date() << std::endl
                       << indent(1) << "</h5>";
            sections.back().add(new CustomHTML(dateheader.str()));
            
            std::stringstream imagetext;
            imagetext << "<a href=\"" << posts[i]->url << "\">" << std::endl
                      << indent(2) << "<img class=\"articleimage\" src=\"/images/" << make_simple(posts[i]->shorttitle) << "_ban.png\" alt=\""
                      << posts[i]->title << "\">" << std::endl
                      << indent(1) << "</a>";
            sections.back().add(new CustomHTML(imagetext.str()));
            
            std::stringstream descriptiontext;
            descriptiontext << posts[i]->description << " <a href=\"" << posts[i]->url << "\">Read more...</a>";
            sections.back().add(new Paragraph(descriptiontext.str()));
            i++;
        }
    }
    
    void print(std::string filename = "") {
        std::stringstream temp;
        temp << "index" << (page_num?std::to_string(page_num+1):"") << ".html";
        std::ofstream os(siteroot+temp.str());
        
        print_template(os, 0);
        os << "    <meta name=\"description\" content=\"" << clear_html(description) << "\">" << std::endl;
        print_template(os, 1);
        print_template(os, 2);
        for(auto s : sections) {
            os << s;
        }
        
        if(last_page > 1) {
            os << indent() << "<section class=\"index\" id=\"indexnav\">" << std::endl
               << indent(1) << "<div";
               
            if(0 < page_num && page_num < last_page-1) { /* double */
                os << " id=\"navdouble\">" << std::endl;
                os << indent(2) << "<a href=\"" << previous_page() << "\" class=\"menubutton\">Newer posts</a>" << std::endl;
                os << indent(2) << "<a href=\"" << next_page() << "\" class=\"menubutton\">Older posts</a>" << std::endl;
            }
            else if(0 < page_num) {
                os << " id=\"navsingle\">" << std::endl;
                os << indent(2) << "<a href=\"" << previous_page() << "\" class=\"menubutton\">Newer posts</a>" << std::endl;
            }
            else if(page_num < last_page-1) {
                os << " id=\"navsingle\">" << std::endl;
                os << indent(2) << "<a href=\"" << next_page() << "\" class=\"menubutton\">Older posts</a>" << std::endl;
            }
            
            os << indent(1) << "</div>" << std::endl
               << indent() << "</section>" << std::endl;
        }
        
        print_template(os, 3);
        print_template(os, 4);
        print_template(os, 5);
    }
};

class Site {
    std::vector<Article*> articles;
    std::vector<Project*> completed_projects;
    std::vector<Project*> abandoned_projects;
    std::vector<Page*> posts;
    std::vector<CustomPage*> custom_pages;
    
    void sort(std::vector<Page*>& vec) {
        for(unsigned i = 0; i < vec.size(); i++) {
            unsigned max_index = i;
            for(unsigned j = i+1; j < vec.size(); j++) {
                if(*vec[j] > *vec[max_index])
                    max_index = j;
            }
            if(max_index != i) {
                Page* temp = vec[i];
                vec[i] = vec[max_index];
                vec[max_index] = temp;
            }
        }
    }
    
    void sort(std::vector<Project*>& vec) {
        for(unsigned i = 0; i < vec.size(); i++) {
            unsigned max_index = i;
            for(unsigned j = i+1; j < vec.size(); j++) {
                if(*vec[j] > *vec[max_index])
                    max_index = j;
            }
            if(max_index != i) {
                Project* temp = vec[i];
                vec[i] = vec[max_index];
                vec[max_index] = temp;
            }
        }
    }
    
public:
    Site() {
        prepare_templates();
        site_posts = &posts;
        
        std::vector<std::string> article_files = get_source_files("articles/");
        std::vector<std::string> a_project_files = get_source_files("projects/abandoned/");
        std::vector<std::string> c_project_files = get_source_files("projects/completed/");
        
        for(auto a : article_files) {
            std::string path = sourceroot + "articles/";
            articles.push_back(new Article(path+a));
        }
        
        for(auto p : c_project_files) {
            std::string path = sourceroot + "projects/completed/";
            completed_projects.push_back(new Project(path+p));
        }
        
        for(auto p : a_project_files) {
            std::string path = sourceroot + "projects/abandoned/";
            abandoned_projects.push_back(new Project(path+p));
        }
        
        //articles.push_back(new Article("doxy.txt"));
        //completed_projects.push_back(new Project("prime.txt"));
        
        sort(posts);
        sort(completed_projects);
        sort(abandoned_projects);
        
        custom_pages.push_back(new ProjectsPage(completed_projects, abandoned_projects));
        custom_pages.push_back(new ArchivesPage(posts));
        custom_pages.push_back(new AboutPage());
        custom_pages.push_back(new LostPage());
        
        unsigned index_pages = posts.size()/10;
        if(index_pages*10 < posts.size())
            index_pages++;
            
        for(unsigned i = 0; i < index_pages; i++) {
            custom_pages.push_back(new IndexPage(posts, i));
        }
        
        //articles[0]->title_debug();
        //completed_projects[1]->title_debug();
        
        print_all();
    }
    
    ~Site() {
        for(auto a : articles)
            delete a;
        for(auto p : completed_projects)
            delete p;
        for(auto p : abandoned_projects)
            delete p;
        for(auto c : custom_pages)
            delete c;
    }
    
    void print_all() {
        makedir(siteroot);
        makedir(siteroot+"articles");
        makedir(siteroot+"projects");
        
        for(auto a : articles)
            a->print();
        for(auto p : completed_projects) {
            makedir(siteroot+p->relativesitepath);
            p->print();
        }
        for(auto p : abandoned_projects) {
            makedir(siteroot+p->relativesitepath);
            p->print();
        }
        for(auto c : custom_pages)
            c->print();
    }
};

int main() {
    Site s;
}
