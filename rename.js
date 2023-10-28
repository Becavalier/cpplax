const fs = require('fs');
const path = require('path');
const folderName = 'operator'
const folderPath = path.resolve(__dirname, `./tests/${folderName}`); // 替换为实际的文件夹路径

fs.readdir(folderPath, (err, files) => {
  if (err) {
    console.error('Error reading directory:', err);
    return;
  }
  
  files.forEach(file => {
    const filePath = path.join(folderPath, file);
    const stat = fs.statSync(filePath);
    
    if (stat.isFile()) {
       let newName = file.replace(/_/g, '-'); // 将下划线替换为连字符
       newName = newName.replace(/lox/g, 'lax')
      const newFilePath = path.join(folderPath, newName);
      
      fs.rename(filePath, newFilePath, (err) => {
        if (err) {
          console.error('Error renaming file:', err);
        } else {
          console.log(`set_property(TEST ${folderName}/${newName} PROPERTY PASS_REGULAR_EXPRESSION "")`)
        }
      });
    }
  });
});