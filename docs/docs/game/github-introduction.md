---
title: MiniOB GitHub 在训练营中的使用说明
---

# MiniOB GitHub 在训练营中的使用说明

## 简介

本文介绍如何在GitHub上创建私有仓库，并将MiniOB代码提交到自己的仓库中，然后在训练营中进行测试。

MiniOB 仓库地址：<https://github.com/oceanbase/miniob>

训练营地址：<https://open.oceanbase.com/train>

## GitHub 私有仓库创建

**NOTE** 一定要使用私有仓库进行测试。

首先你要有一个自己的GitHub账号。账号的注册流程比较简单，没有账号的同学登录GitHub网站创建即可。

1. 登录GitHub网站，并登录自己的账号

2. 在网站右上角点击 "Import Repository"

![Import Repository](images/github-import-repository.png)

3. 填写导入表单

在"Your old repository's clone URL"项目中填写仓库的地址：

```
https://github.com/oceanbase/miniob
```

![fill out begin import form](images/github-begin-import-form.png)

4. 点击"Begin import"按钮，开始导入仓库

导入会花费一定时间，导入完成后，可以看到：

![import done](images/github-import-done.png)

5. 查看仓库首页

点击"Code" 标签，就可以进入到仓库首页。

![view repo](images/github-view-repo.png)

## 仓库权限设置

到此为止，已经创建完了私有仓库，并导入了代码。但是此时官方测试账号并没有权限查看你的代码，接下来需要给官方测试账号添加权限。有两种方法，一种是生成token，另一种是邀请OceanBase测试账号。

### 生成token
1. 在GitHub页面，点击个人头像，选择"Settings"，进入个人设置页面。
![personal settings](images/github-personal-settings.png)

2. 接着在左侧导航栏中选择"Developer settings"，进入开发者设置页面。
![develop settings](images/github-developer-settings.png)

3. 找到生成新token的页面。
![generate new token](images/github-generate-new-token.png)

4. 填写token的信息，生成token。
![fill out token info](images/github-fill-out-token-form.jpg)

5. 生成token后要第一时间保存，离开页面后将无法再次查看。
![save token](images/github-save-token.png)

6. 验证token是否生效
执行 git clone 命令，URL中带上 username和token，如果能够正常clone，则说明token生效。

```bash
git clone https://`your-github-username`:`github_pat_your-github-token`@github.com/`repo-owner`/`repo-name`
```

> your-github-username 替换成你的GitHub用户名<br/>
> github_pat_your-github-token 替换成你的生成的token<br/>
> repo-owner 替换成你的仓库所属者，通常也是你的GitHub用户名<br/>
> repo-name 替换成你的仓库名，比如 miniob-2024

比如：

```bash
git clone https://hnwyllmm:github_pat_token@github.com/hnwyllmm/miniob-2023
```

一个clone成功的例子
![validate token](images/github-validate-token.png)

**注意：提测时的URL需要填写带user:token的地址**
![commit](images/github-commit-info.png)

### 邀请OceanBase测试账号

点击"Settings"标签，进入仓库设置页面，点击"Collaborators"标签，进入仓库权限管理页面。

![add people](images/github-add-people.png)

在"Search by username, full name or email address"中输入"oceanbase-ce-game-test"(官方测试账号)，点击"Add collaborator"按钮，即可将OceanBase测试账号添加到仓库中。

![add tester](images/github-add-tester.png)

![add tester2](images/github-add-tester-2.png)

GitHub 在邀请别人加入仓库时，会发送邮件通知被邀请人，需要对方同意后，才能加入。
因此这个步骤需要等待，官方测试人员会定期刷新邮件处理邀请。如果长时间没有看到邀请成功的信息，请联系官方人员。
也可以直接将邀请链接发送给官方人员，邀请链接可以点击"Pending invite" 右侧的方形按钮复制。

![pending invite](images/github-pending-invite.png)

![invite success](images/github-invite-success.png)

## 提交测试

如果是邀请OceanBase测试账号，需要等官网人员通过邀请后，开始提交测试。
训练营的使用说明不再赘述，可以参考 [训练营使用说明](https://ask.oceanbase.com/t/topic/35600372)。
