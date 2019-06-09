// B-tree
// https://en.wikipedia.org/wiki/B-tree

#include <memory>
#include <vector>
#include <cstddef>
#include <iostream>

template<typename T, unsigned int B = 2>
class Set {
private:
    struct Node {
        Node *parent;
        std::vector<std::unique_ptr<Node>> children;
        std::unique_ptr<const T> data;
        const T *mx;

        Node()
            : parent(nullptr)
            , data(nullptr)
            , mx(nullptr) {}

        Node(T elem, Node *par)
            : parent(par)
            , data(new T(elem)) {
            mx = data.get();
        }

    };

    std::unique_ptr<Node> head;
    size_t sz;

    void recalc(Node *p) {
        if (p->children.empty()) {
            return;
        }
        p->mx = p->children.back()->mx;
        for (auto &i : p->children) {
            i->parent = p;
        }
    }

    // эта функция разделяет вершину на 
    // две, если у нее слишком много(больше 2В) детей
    void split(Node *cur) {
        while (cur->children.size() == 2 * B) {
            // если нахожусь в корне, то создаю новый корень
            if (cur->parent == nullptr) {
                std::unique_ptr<Node> new_root(new Node);
                new_root->mx = head->mx;
                new_root->children.emplace_back(std::move(head));
                head = std::move(new_root);
                cur->parent = head.get();
            }
            // иду в родителя, потом создаю у него нового ребенка
            // и переподвешиваю туда половину детей текущего узла
            auto i = cur->parent->children.begin();
            while (i->get() != cur) {
                i++;
            }
            i++;
            int ind = i - cur->parent->children.begin();
            cur->parent->children.emplace(i, new Node);
            Node *to = cur->parent->children[ind].get();
            for (int j = B; j < 2 * B; j++) {
                to->children.emplace_back(std::move(cur->children[j]));
            }
            for (int j = 0; j < B; j++) {
                cur->children.pop_back();
            }
            recalc(to);
            recalc(cur);
            recalc(cur->parent);
            cur = cur->parent;
        }
    }

    // эта функция "ворует" детей у соседа, если у текущей вершины
    // слишком мало детей
    void merge(Node *cur) {
        while (cur->children.size() < B) {
            if (cur->parent == nullptr) {
                if (cur->children.size() == 1 && !cur->children[0]->children.empty()) {
                    head = std::move(head->children[0]);
                    head->parent = nullptr;
                    recalc(head.get());
                }
                return;
            }
            int i = 0;
            while (cur->parent->children[i].get() != cur) {
                i++;
            }
            Node *neigh;
            // сначала пытаюсь украсть ребенка у левого соседа, если
            // вершина и так самый левый ребенок, то у правого,
            // если не получается, то тогда сливаю вершины в одну
            if (i != 0) {
                neigh = cur->parent->children[i - 1].get();
                if (neigh->children.size() > B) {
                    cur->children.insert(cur->children.begin(), std::move(neigh->children.back()));
                    neigh->children.pop_back();
                    recalc(neigh);
                    recalc(cur);
                    return;
                }
                for (int j = 0; j < cur->children.size(); j++) {
                    neigh->children.push_back(std::move(cur->children[j]));
                }
                recalc(neigh);
                cur = cur->parent;
                cur->children.erase(cur->children.begin() + i);
            } else {
                neigh = cur->parent->children[i + 1].get();
                if (neigh->children.size() > B) {
                    cur->children.push_back(std::move(neigh->children[0]));
                    neigh->children.erase(neigh->children.begin());
                    recalc(neigh);
                    recalc(cur);
                    return;
                }
                for (int j = 0; j < neigh->children.size(); j++) {
                    cur->children.push_back(std::move(neigh->children[j]));
                }
                recalc(cur);
                cur = cur->parent;
                cur->children.erase(cur->children.begin() + i + 1);
            }
        }
    }

public:
    Set()
        : head(new Node)
        , sz(0) {}

    template<typename Iter>
    Set(Iter begin, Iter end)
        : Set() {
        while (begin != end) {
            insert(*begin);
            begin++;
        }
    }

    Set(std::initializer_list<T> init)
        : Set(init.begin(), init.end()) {}

    Set(const Set &set)
        : Set() {
        for (const auto &i : set) {
            insert(i);
        }
    }

    Set &operator=(const Set &set) {
        Set tmp(set);
        head.reset(tmp.head.release());
        sz = tmp.sz;
        return *this;
    }

    bool empty() const {
        return sz == 0;
    }

    size_t size() const {
        return sz;
    }

    class iterator {
        friend Set;

    private:
        Node *ptr;

        iterator(Node *p) : ptr(p) {}

    public:
        iterator() {}

        iterator(const iterator &iter)
            : ptr(iter.ptr) {}

        iterator& operator=(const iterator &iter) {
            ptr = iter.ptr;
            return *this;
        }

        bool operator==(iterator iter) const {
            return ptr == iter.ptr;
        }

        bool operator!=(iterator iter) const {
            return !(*this == iter);
        }

        const T& operator*() const {
            return *(ptr->data);
        }

        const std::unique_ptr<const T>& operator->() const {
            return ptr->data;
        }

        iterator operator++() {
            Node *cur = ptr->parent;
            Node *prev = ptr;
            while (cur != nullptr && cur->children.back().get() == prev) {
                prev = cur;
                cur = cur->parent;
            }
            if (cur == nullptr) {
                ptr = prev;
                return *this;
            }
            int i = 0;
            while (cur->children[i].get() != prev) {
                i++;
            }
            cur = cur->children[i + 1].get();
            while (!cur->children.empty()) {
                cur = cur->children[0].get();
            }
            ptr = cur;
            return *this;
        }

        iterator operator++(int) {
            iterator ret = *this;
            ++(*this);
            return ret;
        }

        iterator operator--() {
            if (!ptr->children.empty()) {
                while (!ptr->children.empty()) {
                    ptr = ptr->children.back().get();
                }
                return *this;
            }
            Node *cur = ptr->parent;
            Node *prev = ptr;
            while (cur->children[0].get() == prev) {
                prev = cur;
                cur = cur->parent;
            }
            int i = 0;
            while (cur->children[i].get() != prev) {
                i++;
            }
            cur = cur->children[i - 1].get();
            while (!cur->children.empty()) {
                cur = cur->children.back().get();
            }
            ptr = cur;
            return *this;
        }

        iterator operator--(int) {
            iterator ret = *this;
            --(*this);
            return ret;
        }
    };

    iterator begin() const {
        Node *cur = head.get();
        while (!cur->children.empty()) {
            cur = cur->children[0].get();
        }
        return iterator(cur);
    }

    iterator end() const {
        return iterator(head.get());
    }

    iterator find(T elem) const {
        if (empty()) {
            return end();
        }
        Node *cur = head.get();
        while (!cur->children.empty()) {
            int i = 0;
            while (i < cur->children.size() && *(cur->children[i]->mx) < elem) {
                i++;
            }
            if (i == cur->children.size()) {
                return end();
            }
            cur = cur->children[i].get();
        }
        if (!(*(cur->data) < elem) && !(elem < *(cur->data))) {
            return iterator(cur);
        }
        return end();
    }

    void insert(T elem) {
        if (empty()) {
            sz++;
            head->children.emplace_back(new Node(elem, head.get()));
            recalc(head.get());
            return;
        }
        if (find(elem) != end()) {
            return;
        }
        sz++;
        Node *cur = head.get();
        // сначала ищу куда вставить элемент
        while (!cur->children.empty()) {
            int i = 0;
            while (*(cur->children[i]->mx) < elem && i < cur->children.size() - 1) {
                i++;
            }
            cur = cur->children[i].get();
        }
        cur = cur->parent;
        auto i = cur->children.begin();
        while (i != cur->children.end() && *(i->get()->data) < elem) {
            i++;
        }
        cur->children.emplace(i, new Node(elem, cur));
        // потом поднимаюсь по предкам, чтобы обновить значение
        // максимума в поддеревьях
        Node *pos = cur;
        while (pos != nullptr) {
            recalc(pos);
            pos = pos->parent;
        }
        split(cur);
    }

    void erase(T elem) {
        iterator iter = find(elem);
        if (iter == end()) {
            return;
        }
        sz--;
        Node *cur = iter.ptr->parent;
        auto i = cur->children.begin();
        while (i->get() != iter.ptr) {
            i++;
        }
        cur->children.erase(i);
        Node *pos = cur;
        // обновляю максимумы на пути до корня
        while (pos != nullptr) {
            recalc(pos);
            pos = pos->parent;
        }
        if (sz != 0) {
            merge(cur);
        }
    }

    iterator lower_bound(T elem) const {
        if (empty()) {
            return end();
        }
        if (elem < *begin()) {
            return begin();
        }
        Node *cur = head.get();
        while (!cur->children.empty()) {
            int i = 0;
            while (i < cur->children.size() && *(cur->children[i]->mx) < elem) {
                i++;
            }
            if (i == cur->children.size()) {
                return end();
            }
            cur = cur->children[i].get();
        }
        iterator iter(cur);
        if ((!(*iter < elem) && !(elem < *iter)) || elem < *iter) {
            return iter;
        }
        return ++iter;
    }
};

